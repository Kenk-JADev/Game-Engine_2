/**
 * @file pathfinding.cpp
 */
#include <aether/nav/pathfinding.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace aether::nav {
namespace {

struct Node {
    GridCoord c;
    f32 g = 0;
    f32 f = 0;
    bool operator>(const Node& o) const noexcept { return f > o.f; }
};

f32 heuristic(GridCoord a, GridCoord b) {
    const f32 dx = static_cast<f32>(a.x - b.x);
    const f32 dz = static_cast<f32>(a.z - b.z);
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace

NavGrid bake_nav_grid(const phys::CollisionWorld& world, f32 min_x, f32 min_z, f32 max_x,
                      f32 max_z, f32 cell_size, f32 agent_radius) {
    NavGrid g;
    g.origin_x = min_x;
    g.origin_z = min_z;
    g.cell_size = cell_size > 0.05f ? cell_size : 1.0f;
    g.width = std::max(1, static_cast<i32>(std::ceil((max_x - min_x) / g.cell_size)));
    g.height = std::max(1, static_cast<i32>(std::ceil((max_z - min_z) / g.cell_size)));
    g.walkable.assign(static_cast<usize>(g.width * g.height), 1);

    for (const auto& body : world.bodies()) {
        if (!body.enabled || !body.block_movement || body.type == phys::BodyType::Trigger) {
            continue;
        }
        if (body.kind == phys::ObjectKind::Player || body.kind == phys::ObjectKind::Npc ||
            body.kind == phys::ObjectKind::Enemy) {
            continue; // Agents blockieren Grid nicht dauerhaft
        }
        const auto wb = body.world_bounds();
        // expand by agent radius
        const f32 x0 = wb.min.x - agent_radius;
        const f32 x1 = wb.max.x + agent_radius;
        const f32 z0 = wb.min.z - agent_radius;
        const f32 z1 = wb.max.z + agent_radius;

        const GridCoord c0 = g.world_to_grid(Vec3{x0, 0, z0});
        const GridCoord c1 = g.world_to_grid(Vec3{x1, 0, z1});
        for (i32 z = c0.z; z <= c1.z; ++z) {
            for (i32 x = c0.x; x <= c1.x; ++x) {
                GridCoord c{x, z};
                if (g.in_bounds(c)) {
                    g.walkable[static_cast<usize>(z * g.width + x)] = 0;
                }
            }
        }
    }
    return g;
}

std::vector<Vec3> find_path(const NavGrid& grid, const Vec3& start, const Vec3& goal) {
    GridCoord s = grid.world_to_grid(start);
    GridCoord g = grid.world_to_grid(goal);
    if (!grid.is_walkable(s) || !grid.is_walkable(g)) {
        // Snap to nearest walkable (simple search)
        auto snap = [&](GridCoord c) -> std::optional<GridCoord> {
            if (grid.is_walkable(c)) return c;
            for (i32 r = 1; r < 6; ++r) {
                for (i32 dz = -r; dz <= r; ++dz) {
                    for (i32 dx = -r; dx <= r; ++dx) {
                        GridCoord n{c.x + dx, c.z + dz};
                        if (grid.is_walkable(n)) return n;
                    }
                }
            }
            return std::nullopt;
        };
        auto ss = snap(s);
        auto gg = snap(g);
        if (!ss || !gg) return {};
        s = *ss;
        g = *gg;
    }
    if (s == g) {
        return {grid.grid_to_world(g)};
    }

    const int w = grid.width;
    const int h = grid.height;
    const usize N = static_cast<usize>(w * h);
    std::vector<f32> gscore(N, std::numeric_limits<f32>::infinity());
    std::vector<i32> parent(N, -1);
    auto idx = [w](GridCoord c) { return c.z * w + c.x; };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    gscore[static_cast<usize>(idx(s))] = 0;
    open.push(Node{s, 0, heuristic(s, g)});

    const GridCoord dirs[8] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1},
                               {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    bool found = false;
    while (!open.empty()) {
        Node cur = open.top();
        open.pop();
        if (cur.c == g) {
            found = true;
            break;
        }
        const f32 cg = gscore[static_cast<usize>(idx(cur.c))];
        if (cur.g > cg + 1e-5f) continue;

        for (const auto& d : dirs) {
            GridCoord n{cur.c.x + d.x, cur.c.z + d.z};
            if (!grid.is_walkable(n)) continue;
            // prevent corner cut through walls
            if (d.x != 0 && d.z != 0) {
                if (!grid.is_walkable({cur.c.x + d.x, cur.c.z}) ||
                    !grid.is_walkable({cur.c.x, cur.c.z + d.z})) {
                    continue;
                }
            }
            const f32 step = (d.x != 0 && d.z != 0) ? 1.41421356f : 1.0f;
            const f32 ng = cg + step;
            const usize ni = static_cast<usize>(idx(n));
            if (ng < gscore[ni]) {
                gscore[ni] = ng;
                parent[ni] = idx(cur.c);
                open.push(Node{n, ng, ng + heuristic(n, g)});
            }
        }
    }
    if (!found) return {};

    std::vector<GridCoord> rev;
    GridCoord c = g;
    while (!(c == s)) {
        rev.push_back(c);
        const i32 p = parent[static_cast<usize>(idx(c))];
        if (p < 0) return {};
        c = GridCoord{p % w, p / w};
    }
    rev.push_back(s);
    std::reverse(rev.begin(), rev.end());

    std::vector<Vec3> path;
    path.reserve(rev.size());
    for (const auto& gc : rev) {
        path.push_back(grid.grid_to_world(gc));
    }
    return path;
}

void NavAgent::set_path(std::vector<Vec3> path) {
    path_ = std::move(path);
    index_ = 0;
}

void NavAgent::clear() {
    path_.clear();
    index_ = 0;
}

Vec3 NavAgent::update(const Vec3& current, f32 speed, f64 dt) {
    if (path_.empty() || index_ >= path_.size()) {
        return Vec3(0);
    }
    const Vec3 target = path_[index_];
    Vec3 to = target - current;
    to.y = 0.0f;
    const f32 dist = glm::length(to);
    const f32 step = speed * static_cast<f32>(dt);
    if (dist <= step || dist < 1.0e-3f) {
        ++index_;
        if (index_ >= path_.size()) {
            path_.clear();
            index_ = 0;
        }
        return to;
    }
    return (to / dist) * step;
}

} // namespace aether::nav
