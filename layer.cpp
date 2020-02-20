
#include "4coder_default_include.cpp"
#include "generated/managed_id_metadata.cpp"

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
    
  // Here comes my configuration code...

  mapping_init(tctx, &framework_mapping);
  configure_mappings(&framework_mapping);
}

