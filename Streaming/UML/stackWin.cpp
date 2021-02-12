int main(int argc, char *argv[]) {
	windows_options options;
	windows_osd_interface osd(options);

	if (win_is_gui_application() || is_double_click_start(args.size()))
		FreeConsole();

	osd.register_options();
	return emulator_info::start_frontend(options, osd, args);
}
