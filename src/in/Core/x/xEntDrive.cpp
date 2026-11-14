#include "xCommon.h"
#include "xEntDrive.h"

void xEntDriveInit(xEntDrive* drv, xEnt* driven)
{
    drv->flags = 0;
    drv->driven = driven;
    drv->driver = NULL;
    drv->s = 0.0f;
    drv->tm = 0.0f;
    drv->tmr = 0.0f;
    drv->old_driver = NULL;
    drv->os = 0.0f;
    drv->otm = 0.0f;
    drv->otmr = 0.0f;
}

void xEntDriveMount(xEntDrive* drv, xEnt* driver, F32 mt, const xCollis* coll)
{
    drv->dloc = 0.0f;

    if (driver == drv->old_driver && drv->os) {
        drv->driver = driver;
        driver->driving_count++;
        if (mt < 0.0f) {
            drv->s = 1.0f;
            drv->tmr = 0.0f;
        } else {
            drv->s = drv->os;
            drv->tmr = mt * (1.0f - drv->s);
        }
        drv->tm = mt;
        drv->old_driver = NULL;
        drv->os = 0.0f;
        drv->otm = 0.0f;
        drv->otmr = 0.0f;
    } else {
        drv->driver = driver;
        driver->driving_count++;
        if (mt < 0.0f) {
            drv->s = 1.0f;
            drv->tmr = 0.0f;
        } else {
            drv->s = 0.0f;
            drv->tmr = mt;
        }
        drv->tm = mt;
    }

    if (drv->flags & 0x1) {
        xVec3 euler;
        xMat3x3 a_descaled;
        F32 dummy;
        xVec3NormalizeMacro(&a_descaled.right, &drv->driver->model->Mat->right, &dummy);
        xVec3NormalizeMacro(&a_descaled.up, &drv->driver->model->Mat->up, &dummy);
        xVec3NormalizeMacro(&a_descaled.at, &drv->driver->model->Mat->at, &dummy);
        xMat3x3GetEuler(&a_descaled, &euler);
        drv->yaw_in_driver = euler.x;
    }

    if (coll && (coll->flags & k_HIT_CALC_TRI)) {
        drv->flags |= 0x2;
        (xCollis::tri_data&)drv->tri = coll->tri;
        drv->tri.loc = xCollisTriHit(drv->tri, *driver->model);
        xMat4x3Tolocal(&drv->tri.loc, (xMat4x3*)drv->driver->model->Mat, &drv->tri.loc);
        drv->tri.coll = coll;
        drv->tri.trioldmat = *(xMat4x3*)drv->driver->model->Mat;
    }

    xVec3Copy(&drv->last_driven_pos, &drv->driven->frame->mat.pos);
    xMat4x3Tolocal(&drv->driven_pos_in_driver, (xMat4x3*)drv->driver->model->Mat, &drv->last_driven_pos);
}

void xEntDriveDismount(xEntDrive* drv, F32 dmt)
{
    xEnt* pDriver = drv->driver;
    if (!pDriver) {
        return;
    }

    pDriver->driving_count--;

    drv->old_driver = pDriver;
    drv->os = drv->s;
    drv->otm = dmt;
    drv->otmr = dmt * drv->os;
    drv->driver = NULL;
    drv->s = 0.0f;
    drv->tm = 0.0f;
    drv->tmr = 0.0f;
    drv->flags &= ~0x2;

    xVec3Copy(&drv->last_driven_pos, &drv->driven->frame->mat.pos);
    xMat4x3Tolocal(&drv->driven_pos_in_old_driver, (xMat4x3*)drv->old_driver->model->Mat, &drv->last_driven_pos);
}

void xEntDriveUpdate(xEntDrive* drv, xScene*, F32 dt, const xCollis*)
{
    if (!drv->old_driver && !drv->driver) {
        return;
    }

    if (drv->otmr > 0.0f) {
        drv->otmr -= dt;
        if (drv->otmr <= 0.0f) {
            drv->os = 0.0f;
            drv->otmr = 0.0f;
        }
        else if (xfeq0(drv->otm))
            drv->os = 0.0f;
        else
            drv->os = drv->otmr / drv->otm;
    } else {
        drv->os = 0.0f;
    }

    if (drv->tmr > 0.0f) {
        drv->tmr -= dt;
        if (drv->tmr <= 0.0f) {
            drv->s = 1.0f;
            drv->tmr = 0.0f;
        } else {
            drv->s = 1.0f - drv->tmr / drv->tm;
        }
    }

    if (!drv->os && !drv->s) {
        return;
    }

    if (drv->s && (drv->flags & 0x1)) {
        xVec3 euler;
        xMat3x3 rot, a_descaled;
        F32 dummy;
        xVec3NormalizeMacro(&a_descaled.right, &drv->driver->model->Mat->right, &dummy);
        xVec3NormalizeMacro(&a_descaled.up, &drv->driver->model->Mat->up, &dummy);
        xVec3NormalizeMacro(&a_descaled.at, &drv->driver->model->Mat->at, &dummy);
        xMat3x3GetEuler(&a_descaled, &euler);
        xMat3x3RotY(&rot, drv->s * (euler.x - drv->yaw_in_driver));
        xMat3x3Mul(&drv->driven->frame->mat, &drv->driven->frame->mat, &rot);
        drv->yaw_in_driver = euler.x;
    }

    drv->dloc = 0.0f;

    xVec3 newq;

    if (drv->os && drv->old_driver) {
        xMat4x3Toworld(&newq, (xMat4x3*)drv->old_driver->model->Mat, &drv->driven_pos_in_old_driver);
        xVec3Sub(drv->driven->frame->setDpos(), newq, drv->last_driven_pos);
        xVec3SMulBy(drv->driven->frame->setDpos(), drv->os);
        xVec3AddTo(drv->driven->frame->mat.pos, drv->driven->frame->getDpos());
        drv->dloc += drv->driven->frame->getDpos();
    }

    if (drv->s) {
        if (drv->flags & 0x2) {
            xModelInstance& m = *drv->driver->model;
            if (xModelAnimCollDirty(m)) xModelAnimCollRefresh(m);

            xVec3 world_loc, new_loc;
            xMat4x3Toworld(&world_loc, &drv->tri.trioldmat, &drv->tri.loc);
            new_loc = xCollisTriHit(drv->tri, m);
            drv->driven->frame->setDpos() = new_loc - world_loc;

            if (drv->tri.index != drv->tri.coll->tri.index
                || !xeq(drv->tri.r, drv->tri.coll->tri.r, 0.1f)
                || !xeq(drv->tri.d, drv->tri.coll->tri.d, 0.1f)) {
                (xCollis::tri_data&)drv->tri = drv->tri.coll->tri;
            }

            xMat4x3 oldmat = *(xMat4x3*)m.Mat;
            *(xMat4x3*)m.Mat = *(xMat4x3*)drv->driver->model->Mat;
            drv->tri.loc = xCollisTriHit(drv->tri, m);
            xMat4x3Tolocal(&drv->tri.loc, (xMat4x3*)drv->driver->model->Mat, &drv->tri.loc);
            *(xMat4x3*)m.Mat = oldmat;
        } else {
            xMat4x3Toworld(&newq, (xMat4x3*)drv->driver->model->Mat, &drv->driven_pos_in_driver);
            xVec3Sub(drv->driven->frame->setDpos(), newq, drv->last_driven_pos);
        }

        drv->driven->frame->setDpos() *= drv->s;
        drv->dloc += drv->driven->frame->getDpos();
        drv->driven->frame->mat.pos += drv->driven->frame->getDpos();
        drv->driven->frame->setDpos().assign(0.0f);

        if (drv->driven->model) {
            (xVec3&)drv->driven->model->Mat->pos = drv->driven->frame->mat.pos;
        }
    }

    xVec3Copy(&drv->last_driven_pos, &drv->driven->frame->mat.pos);

    if (drv->os) {
        xMat4x3Tolocal(&drv->driven_pos_in_old_driver, (xMat4x3*)drv->old_driver->model->Mat, &drv->last_driven_pos);
    }

    if (drv->s) {
        xMat4x3Tolocal(&drv->driven_pos_in_driver, (xMat4x3*)drv->driver->model->Mat, &drv->last_driven_pos);
        drv->tri.trioldmat = *(xMat4x3*)drv->driver->model->Mat;
    }
}
