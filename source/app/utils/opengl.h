#pragma once

#define REPORT_GL_MAX(id) 	{\
int aMax;\
glGetIntegerv(id, &aMax);\
LOGI("OpenGL Max " #id ": %d", aMax);\
}
