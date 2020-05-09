
#include "4coder_default_include.cpp"

#include "rendering.cpp"
#include "generated/managed_id_metadata.cpp"

// TODOs:

//   Change rendering mode to have a smooth scrolling rather then jumping by line

//   Change the default size of the window on start to something bigger and better. Wonder if there's a way to center it somehow as well?
//   Would be nice to tweak theme colours a bit, especially scope highlighting
//   Turn off current line highlighting
// 
// Functions:
//   join lines forward

CUSTOM_COMMAND_SIG(custom_startup) 
{
    ProfileScope(app, "Startup");
    User_Input input = get_current_input(app);
    if (match_core_code(&input, CoreCode_Startup)){
        String_Const_u8_Array file_names = input.event.core.file_names;
        load_themes_default_folder(app);
        default_4coder_initialize(app, file_names);
        default_4coder_one_panel(app);
        if (global_config.automatically_load_project){
            load_project(app);
        }
    }
}

// NOTE: To have custom bindings working set `mappings` in config.4coder to ""
static void configure_mappings (Mapping *mapping) {
  #include "bindings.cpp"
}

void custom_layer_init (Application_Links *app) {
  Thread_Context *tctx = get_thread_context(app);
    
  // NOTE(allen): setup for default framework
  async_task_handler_init(app, &global_async_system);
  code_index_init();
  buffer_modified_set_init();
  Profile_Global_List *list = get_core_profile_list(app);
  ProfileThreadName(tctx, list, string_u8_litexpr("main"));
  initialize_managed_id_metadata(app);
  set_default_color_scheme(app);
      
  // NOTE(allen): default hooks and command maps
  set_all_default_hooks(app);
  set_custom_hook(app, HookID_RenderCaller, custom_render_caller);
    
  // Here comes my configuration code...
    
  mapping_init(tctx, &framework_mapping);
  configure_mappings(&framework_mapping);
}

