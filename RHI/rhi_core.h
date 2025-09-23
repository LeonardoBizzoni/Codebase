#ifndef RHI_CORE_H
#define RHI_CORE_H

typedef struct {
  u64 h[1];
} RHI_Handle;

typedef u8 RHI_ShaderType;
enum {
  RHI_ShaderType_Vertex,
  RHI_ShaderType_Fragment,
};

fn void rhi_init(void);

#endif
