TEMPLATE = subdirs

CONFIG += debug

CONFIG(debug, debug|release) {
    message("$${TARGET} - debug mode")
}else {
    message("$${TARGET} - release mode")
}

SUBDIRS = piHomeCommon \
          dataManager \
          userManager \
          sensorManager \
          alarmManager \
          webManager \
          uiManager

dataManager.depends = piHomeCommon
sensorManager.depends = dataManager
userManager.depends = piHomeCommon
alarmManager.depends = dataManager
alarmManager.depends = piHomeCommon
uiManager.depends = userManager
uiManager.depends = sensorManager
uiManager.depends = alarmManager
webManager.depends = userManager
webManager.depends = sensorManager
webManager.depends = alarmManager