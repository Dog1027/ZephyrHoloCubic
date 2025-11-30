
if(CONFIG_APP_MENU)
    target_sources(app PRIVATE bg/app_menu.c)
endif()

target_sources(app PRIVATE fg/app_test.c)
