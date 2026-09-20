#include "splinetexture.h"
#include "splinetexturegpu.h"
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFramebufferObject>
#include <QDebug>
#include <cmath>
#include <cstdio>
#include <vector>
int main(int argc, char **argv) {
 QGuiApplication app(argc,argv);
 QSurfaceFormat fmt; fmt.setVersion(3,3); fmt.setProfile(QSurfaceFormat::CoreProfile);
 QOpenGLContext ctx; ctx.setFormat(fmt); if(!ctx.create()){ puts("SKIP: OpenGL 3.3 context unavailable"); return 77; }
 QOffscreenSurface surf; surf.setFormat(ctx.format()); surf.create(); if(!ctx.makeCurrent(&surf)){ puts("current fail"); return 2; }
 QOpenGLFunctions_3_3_Core gl; gl.initializeOpenGLFunctions();
 printf("GL: %s\n",gl.glGetString(GL_RENDERER));
 QOpenGLFramebufferObjectFormat ff; ff.setInternalTextureFormat(GL_R32F); QOpenGLFramebufferObject fbo(1024,128,ff);
 XmbSpline::GpuGenerator gen;
 if(!gen.initialize(&gl,fbo.texture())){ puts("initialize fail"); return 2; }
 gl.glBindFramebuffer(GL_FRAMEBUFFER,fbo.handle());
 std::vector<float> cpu(XmbSpline::texelCount), gpu(XmbSpline::texelCount);
 double maxerr=0, sumsq=0; size_t n=0;
 for(int i=0;i<65;++i){ float flow=float(i)*0.37f; XmbSpline::generate(flow,cpu.data()); if(!gen.render(flow)){ puts("render fail"); return 3; } gl.glReadPixels(0,0,1024,128,GL_RED,GL_FLOAT,gpu.data()); for(size_t j=0;j<cpu.size();++j){ double e=std::abs(double(cpu[j])-gpu[j]); if(!std::isfinite(e)) return 3; maxerr=std::max(maxerr,e); sumsq+=e*e; ++n; } }
 printf("texels %zu max_abs %.9g RMS %.9g\n",n,maxerr,std::sqrt(sumsq/double(n)));
 gen.cleanup();
 return maxerr>1e-5?1:0;
}
