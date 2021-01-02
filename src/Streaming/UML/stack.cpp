int main(int argc, char** argv) {
	sdl_options options;
	sdl_osd_interface osd(options);
	return emulator_info::start_frontend(options, osd, args);
}

int emulator_info::start_frontend(emu_options &options, osd_interface &osd, vector<string> &args) {
	cli_frontend frontend(options, osd);
	return frontend.execute(args);
}

int cli_frontend::execute(vector<string> &args) {
	mame_machine_manager manager(m_options, m_osd);
	start_execution(manager, args);
}

void cli_frontend::start_execution(mame_machine_manager *manager, const vector<string> &args) {
	manager->execute();
}

int mame_machine_manager::execute() {
	machine_config config(*system, m_options);
	running_machine machine(config, *this);
	machine.run(is_empty);
}

int running_machine::run(bool quiet) {
	start();
	manager().ui_initialize(*this);
	
	while ((!m_hard_reset_pending && !m_exit_pending) || m_saveload_schedule != saveload_schedule::NONE) {		
		if (!m_paused)
			m_scheduler.timeslice();
		else
			m_video->frame_update();
	}
}

void running_machine::start() {
	m_manager.osd().init(*this);
	
	video_manager m_video(*this);
	sound_manager m_sound(*this);

	m_ui = manager().create_ui(*this);
}

void sdl_osd_interface::init(running_machine &machine) {
	osd_common_t::init_subsystems();
}

void osd_common_t::init_subsystems() {
	video_init();
}

bool sdl_osd_interface::video_init() {
	window_init();
}

bool sdl_osd_interface::window_init() {
	renderer_sdl2::init(machine());
}

void renderer_sdl2::init(running_machine &machine) {
	SDL_GL_LoadLibrary(stemp);
}

void mame_machine_manager::ui_initialize(running_machine& machine) {
	m_ui->display_startup_screens(m_firstrun);
}

void mame_ui_manager::display_startup_screens(bool first_time) {
	show_menu();
}

void mame_ui_manager::show_menu() {
	set_handler(ui_callback_type::MENU, bind(&ui::menu::ui_handler, _1, ref(*this)));
}

void video_manager::frame_update(bool from_debugger) {
	emulator_info::draw_user_interface(machine());
	
	machine().osd().update(!from_debugger && skipped_it);
	machine().osd().input_update();		
}

void emulator_info::draw_user_interface(running_machine& machine) {
	mame_machine_manager::instance()->ui().update_and_render(machine.render().ui_container());
}

void mame_ui_manager::update_and_render(render_container &container) {
	m_handler_callback(container) = [this] (render_container &container) -> uint32_t
	{
		draw_text_box(container, messagebox_text, ui::text_layout::LEFT, 0.5f, 0.5f, colors().background_color());
		return 0;
	})
}

void sdl_osd_interface::update(bool skip_redraw) {
	for (auto window : osd_common_t::s_window_list)
		window->update();
}

int sdl_window_info::window_init() {
	set_renderer(osd_renderer::make_for_type(video_config.mode, static_cast<osd_window*>(this)->shared_from_this()));
}

unique_ptr<osd_renderer> osd_renderer::make_for_type(int mode, shared_ptr<osd_window> window, int extra_flags) {
	return make_unique<renderer_sdl2>(window, extra_flags);
}

void sdl_window_info::update() {
	renderer().draw(update);
}

int renderer_sdl2::create() {
	auto win = assert_window();
	m_sdl_renderer = SDL_CreateRenderer(dynamic_pointer_cast<sdl_window_info>(win)->platform_window(), -1, SDL_RENDERER_ACCELERATED);
}

int renderer_sdl2::draw(int update) {
	auto win = assert_window();
	
	win->m_primlist->acquire_lock();
	
	for (render_primitive &prim : *win->m_primlist)
		switch (prim.type)
		{
			case render_primitive::LINE:
				SDL_RenderDrawLine();
			case render_primitive::QUAD:
				render_quad();	
		}
		
	win->m_primlist->release_lock();
	SDL_RenderPresent(m_sdl_renderer);
}
