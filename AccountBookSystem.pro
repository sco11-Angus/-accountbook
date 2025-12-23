QT       += core gui sql widgets network

CONFIG += c++17


SOURCES += \
    account_add_widget.cpp \
    account_list_widget.cpp \
    account_manager.cpp \
    account_record.cpp \
    accountbookmainwidget.cpp \
    accountbookrecordwidget.cpp \
    ai_manager.cpp \
    bill_handler.cpp \
    bill_sync_client.cpp \
    business_logic.cpp \
    main.cpp \
    mainwindow.cpp \
    mysql_helper.cpp \
    server_main.cpp \
    sqlite_helper.cpp \
    statistics_manager.cpp \
    sync_manager.cpp \
    tcp_server.cpp \
    tcpclient.cpp \
    thread_manager.cpp \
    ui_handler.cpp \
    user.cpp \
    user_handler.cpp \
    user_info_widget.cpp \
    user_manager.cpp \
    user_sync_client.cpp \
    LoginWidget.cpp \
    RegisterWidget.cpp

HEADERS += \
    account_add_widget.h \
    account_list_widget.h \
    account_manager.h \
    account_record.h \
    accountbookmainwidget.h \
    accountbookrecordwidget.h \
    ai_manager.h \
    bill_handler.h \
    bill_sync_client.h \
    business_logic.h \
    mainwindow.h \
    mysql_helper.h \
    server_main.h \
    sqlite_helper.h \
    statistics_manager.h \
    sync_manager.h \
    tcp_server.h \
    tcpclient.h \
    thread_manager.h \
    ui_handler.h \
    user.h \
    user_handler.h \
    user_info_widget.h \
    user_manager.h \
    user_sync_client.h \
    LoginWidget.h \
    RegisterWidget.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
