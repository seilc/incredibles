#include "xCommon.h"
#include "zNPCWaterhose.h"

namespace zNPC
{
    void water_bone::setup()
    {
        waterhose = NULL;
        setup_base("Water");
    }

    void water_bone::reset()
    {
        turn_off();
    }

    void water_bone::turn_on()
    {
        if (waterhose == NULL) {
            waterhose = new zWaterhose(false, false);
        }
    }

    void water_bone::turn_off()
    {
        if (waterhose != NULL) {
            waterhose->ShutDown();
            waterhose->DoMgrUpdate(true);
            waterhose->DoMgrRender(true);
            waterhose = NULL;
        }
    }

    void water_bone::update(F32 dt)
    {
        xMat4x3 mat;
        xMat3x3Copy(&mat, (xMat4x3*)&owner->model->Mat[aim_bone + 1]);
        xMat3x3Mul(&mat, &mat, (xMat4x3*)owner->model->Mat);
        mat.pos = get_position();
        waterhose->Update(dt, mat);
    }

    bool water_bone::runnable(F32)
    {
        return waterhose != NULL;
    }

    void water_bone::render_extra()
    {
        if (waterhose != NULL) {
            waterhose->Render();
        }
    }
} // namespace zNPC
