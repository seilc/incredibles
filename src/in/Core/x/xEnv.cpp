#include "xEnv.h"

#include "xstransvc.h"

xEnv* gCurXEnv = NULL;

void xEnvLoadJSPList(xEnv* env, S32 dataType)
{
    if (dataType == 0)
    {
        U32 bufsize;
        void* buf;

        env->geom = &env->ienv;

        S32 jsp_count = xSTAssetCountByType('JSP ');

        iEnvLoadBegin(env->geom, dataType, jsp_count / 2);

        S32 index = 0;

        for (S32 i = 0; i < jsp_count; i++)
        {
            buf = xSTFindAssetByType('JSP ', i, &bufsize);

            PKRAssetTOCInfo info;

            xSTGetAssetInfoByType('JSP ', i, &info);

            bool32 rc = iEnvLoadJSP(env->geom, info.aid, buf, bufsize, dataType, index);

            if (rc)
            {
                index++;
            }
        }

        env->geom->jsp_count = index;

        iEnvLoadEnd(env->geom, dataType);
    }

    gCurXEnv = env;
}

void xEnvSetup(xEnv* env)
{
    iEnvSetup(env->geom);
    iEnvDefaultLighting(env->geom);

    env->lightKit = NULL;

    gCurXEnv = env;
}

void xEnvFree(xEnv* env)
{
    if (env->geom)
    {
        iEnvFree(env->geom);

        env->geom = NULL;
    }
}

void xEnvRender(xEnv* env, bool alpha)
{
    if (env->geom)
    {
        iEnvRender(env->geom, alpha);
    }

    if (env->geom->jsp_count > 0)
    {
        for (S32 i = 0; i < env->geom->jsp_count; i++)
        {
            // do nothing
        }

        if (env->geom->jsp_count == 1)
        {
            env->geom->jsp_visibilityCount[0] = 1;
        }
    }
}
