#include "ui/theme.h"

const Theme& Theme::Default() {
    static Theme theme;
    return theme;
}
