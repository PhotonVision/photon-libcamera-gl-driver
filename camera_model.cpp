#include "camera_model.h"
#include <cstring>

CameraModel stringToModel(const std::string &model) {
    printf("Checking model: %s\n", model.c_str());
    const char *famname = model.c_str();
    if (!strcmp(famname, "ov5647"))
        return OV5647;
    else if (!strcmp(famname, "imx219"))
        return IMX219;
    else if (!strcmp(famname, "imx477"))
        return IMX477;
    else if (!strcmp(famname, "ov9281"))
        return OV9281;
    else if (!strcmp(famname, "ov7251"))
        return OV7251;
    else if (!strcmp(famname, "Disconnected"))
        return Disconnected;
    else
        return Unknown;
}
