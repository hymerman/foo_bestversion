#include "FoobarSDKWrapper.h"

#include "TextToPlaylistDialogue.h"

// {DC74D28F-65FB-4ECB-8D96-F5AD0F276BED}
static const GUID g_mainmenu_group_id = { 0xdc74d28f, 0x65fb, 0x4ecb, { 0x8d, 0x96, 0xf5, 0xad, 0xf, 0x27, 0x6b, 0xed } };

static mainmenu_group_popup_factory g_mainmenu_group(g_mainmenu_group_id, mainmenu_groups::file, static_cast<t_uint32>(mainmenu_commands::sort_priority_dontcare), "foo_bestversion");

namespace bestversion {

class FooBestVersionMainMenu : public mainmenu_commands {
public:
	enum {
		cmd_text_to_playlist_dialogue = 0,
		cmd_total
	};
	t_uint32 get_command_count() {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) {
		// {4CFBDB19-50D9-4CD0-A8AA-1D530F8E8EF9}
		static const GUID guid_text_to_playlist_dialogue = { 0x4cfbdb19, 0x50d9, 0x4cd0, { 0xa8, 0xaa, 0x1d, 0x53, 0xf, 0x8e, 0x8e, 0xf9 } };
		switch(p_index) {
			case cmd_text_to_playlist_dialogue: return guid_text_to_playlist_dialogue;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_text_to_playlist_dialogue: p_out = "Text to playlist..."; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_text_to_playlist_dialogue: p_out = "Generates a playlist from multiline text input."; return true;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	GUID get_parent() {
		return g_mainmenu_group_id;
	}
	void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback) {
		switch(p_index) {
			case cmd_text_to_playlist_dialogue:
				showTextToPlaylistDialogue();
				break;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

static mainmenu_commands_factory_t<FooBestVersionMainMenu> g_FooBestVersionMainMenu_factory;

} // namespace bestversion
