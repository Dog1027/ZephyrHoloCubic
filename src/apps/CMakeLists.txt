
if(CONFIG_APP_MENU)
    target_sources(app PRIVATE bg/app_menu.c)
endif()

target_sources(app PRIVATE fg/app_test.c)
target_sources(app PRIVATE fg/app_files.c)
target_sources(app PRIVATE fg/app_anim.c)
target_sources(app PRIVATE fg/app_photos.c)
target_sources(app PRIVATE fg/app_level.c)
target_sources(app PRIVATE fg/app_dice.c)
target_sources(app PRIVATE fg/app_light.c)
target_sources(app PRIVATE fg/app_timer.c)
target_sources(app PRIVATE fg/app_sysinfo.c)
