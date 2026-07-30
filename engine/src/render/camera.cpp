/**
 * @file camera.cpp
 */
#include <aether/render/camera.hpp>

namespace aether::render {
namespace {

Vec4 normalize_plane(const Vec4& p) {
    const f32 len = glm::length(Vec3(p));
    if (len <= 1.0e-8f) {
        return p;
    }
    return p / len;
}

} // namespace

Frustum extract_frustum(const Mat4& vp) {
    Frustum f;
    // Gribb/Hartmann: Zeilen der kombinierten Matrix
    const Vec4 row0(vp[0][0], vp[1][0], vp[2][0], vp[3][0]);
    const Vec4 row1(vp[0][1], vp[1][1], vp[2][1], vp[3][1]);
    const Vec4 row2(vp[0][2], vp[1][2], vp[2][2], vp[3][2]);
    const Vec4 row3(vp[0][3], vp[1][3], vp[2][3], vp[3][3]);

    f.planes[Frustum::Left]   = normalize_plane(row3 + row0);
    f.planes[Frustum::Right]  = normalize_plane(row3 - row0);
    f.planes[Frustum::Bottom] = normalize_plane(row3 + row1);
    f.planes[Frustum::Top]    = normalize_plane(row3 - row1);
    f.planes[Frustum::Near]   = normalize_plane(row3 + row2);
    f.planes[Frustum::Far]    = normalize_plane(row3 - row2);
    return f;
}

void Camera::set_perspective(f32 fov_y_degrees, f32 aspect, f32 near_plane, f32 far_plane) {
    proj_type_ = Projection::Perspective;
    fov_y_ = fov_y_degrees;
    aspect_ = aspect > 1.0e-6f ? aspect : 1.0f;
    near_ = near_plane;
    far_ = far_plane;
    dirty_ = true;
}

void Camera::set_orthographic(f32 left, f32 right, f32 bottom, f32 top,
                              f32 near_plane, f32 far_plane) {
    proj_type_ = Projection::Orthographic;
    ortho_l_ = left;
    ortho_r_ = right;
    ortho_b_ = bottom;
    ortho_t_ = top;
    near_ = near_plane;
    far_ = far_plane;
    dirty_ = true;
}

void Camera::look_at(const Vec3& eye, const Vec3& center, const Vec3& up) {
    position_ = eye;
    target_ = center;
    up_ = up;
    dirty_ = true;
}

void Camera::recompute() const {
    if (!dirty_) {
        return;
    }
    view_ = glm::lookAt(position_, target_, up_);
    if (proj_type_ == Projection::Perspective) {
        proj_ = glm::perspective(radians(fov_y_), aspect_, near_, far_);
    } else {
        proj_ = glm::ortho(ortho_l_, ortho_r_, ortho_b_, ortho_t_, near_, far_);
    }
    dirty_ = false;
}

const Mat4& Camera::view_matrix() const {
    recompute();
    return view_;
}

const Mat4& Camera::projection_matrix() const {
    recompute();
    return proj_;
}

Mat4 Camera::view_projection_matrix() const {
    return projection_matrix() * view_matrix();
}

Frustum Camera::frustum() const {
    return extract_frustum(view_projection_matrix());
}

} // namespace aether::render
