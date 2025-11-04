fn void rhi_camera_update(RHI_Camera *camera) {
  Vec3F32 new_front = {0};
  new_front.x = cosf(camera->yaw * (f32)M_PI / 180.f) *
                cosf(camera->pitch * (f32)M_PI / 180.f);
  new_front.y = sinf(camera->pitch * (f32)M_PI / 180.f);
  new_front.z = sinf(camera->yaw * (f32)M_PI / 180.f) *
                cosf(camera->pitch * (f32)M_PI / 180.f);
  camera->direction_front = vec3f32_normalize(new_front);
  camera->direction_right = vec3f32_normalize(vec3f32_cross(camera->direction_front,
                                                            camera->direction_up_world));
  camera->direction_up = vec3f32_normalize(vec3f32_cross(camera->direction_right,
                                                         camera->direction_front));
}

fn Mat4F32 rhi_camera_lookat(RHI_Camera *camera) {
  Mat4F32 res = {0};
  res.values[0][0] =  camera->direction_right.x;
  res.values[0][1] =  camera->direction_up.x;
  res.values[0][2] = -camera->direction_front.x;
  res.values[1][0] =  camera->direction_right.y;
  res.values[1][1] =  camera->direction_up.y;
  res.values[1][2] = -camera->direction_front.y;
  res.values[2][0] =  camera->direction_right.z;
  res.values[2][1] =  camera->direction_up.z;
  res.values[2][2] = -camera->direction_front.z;
  res.values[3][0] = -vec3f32_dot(&camera->direction_right, &camera->position);
  res.values[3][1] = -vec3f32_dot(&camera->direction_up, &camera->position);
  res.values[3][2] =  vec3f32_dot(&camera->direction_front, &camera->position);
  res.values[3][3] =  1.0f;
  return res;
}
