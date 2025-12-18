QT       += core gui
QT += sql
QT += core gui widgets core5compat  # 新增 core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    account_add_widget.cpp \
    account_list_widget.cpp \
    account_manager.cpp \
    account_record.cpp \
    ai_manager.cpp \
    main.cpp \
    mainwindow.cpp \
    sqlite_helper.cpp \
    statistics_manager.cpp \
    sync_manager.cpp \
    thread_manager.cpp \
    user.cpp \
    user_info_widget.cpp \
    user_manager.cpp \
    LoginWidget.cpp \
    RegisterWidget.cpp

HEADERS += \
    account_add_widget.h \
    account_list_widget.h \
    account_manager.h \
    account_record.h \
    ai_manager.h \
    mainwindow.h \
    sqlite_helper.h \
    statistics_manager.h \
    sync_manager.h \
    thread_manager.h \
    user.h \
    user_info_widget.h \
    user_manager.h \
    LoginWidget.h \
    RegisterWidget.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
