#include "TextToPlaylistDialogue.h"
#include "FoobarSDKWrapper.h"
#include "ATLHelpersWrapper.h"
#include "BestVersion.h"
#include "DatabaseScopeLock.h"
#include "resource.h"
#include "ToString.h"

#include <regex>

namespace bestversion
{

class TextToPlaylistDialogue : public CDialogImpl<TextToPlaylistDialogue> {
public:
	TextToPlaylistDialogue() : CDialogImpl<TextToPlaylistDialogue>() {}

	enum { IDD = IDD_TEXT_TO_PLAYLIST_DIALOGUE };

	BEGIN_MSG_MAP(TextToPlaylistDialogue)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_ARTIST_REGEX_TEXT, EN_CHANGE, OnRegexChange)
		COMMAND_HANDLER_EX(IDC_TITLE_REGEX_TEXT, EN_CHANGE, OnRegexChange)
		COMMAND_HANDLER_EX(IDC_INPUT_TEXT, EN_CHANGE, OnTextInputChange)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOk)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		SetDlgItemText(IDC_ARTIST_REGEX_TEXT,	_T("[^/]*/([^/]+)/[^/]+/(?:[^/]+/)?[\\d\\s-]*[^/]+\\.(?:mp3|flac|ogg)"));
		SetDlgItemText(IDC_TITLE_REGEX_TEXT,	_T("[^/]*/[^/]+/[^/]+/(?:[^/]+/)?[\\d\\s-]*([^/]+)\\.(?:mp3|flac|ogg)"));
		ShowWindowCentered(*this, GetParent()); // Function declared in SDK helpers.
		return TRUE;
	}

	void OnRegexChange(UINT, int, CWindow)
	{
		handle_regex_changed();
	}

	void OnTextInputChange(UINT, int, CWindow)
	{
		handle_input_text_changed();
	}

	void OnOk(UINT, int, CWindow) {
		DestroyWindow();
	}

	void OnCancel(UINT, int, CWindow) {
		DestroyWindow();
	}

	void handle_regex_changed()
	{
		handle_input_text_changed();
	}

	void handle_input_text_changed()
	{
		console::print("starting matching");
		pfc::string8 artist_regex_string;
		uGetDlgItemText(*this, IDC_ARTIST_REGEX_TEXT, artist_regex_string);
		pfc::string8 title_regex_string;
		uGetDlgItemText(*this, IDC_TITLE_REGEX_TEXT, title_regex_string);

		// Compile regexes.
		try
		{
			std::regex artist_regex(artist_regex_string);
			std::regex title_regex(title_regex_string);
			std::smatch artist_match;
			std::smatch title_match;

			// Split input text into lines.
			pfc::string8 text_input;
			uGetDlgItemText(*this, IDC_INPUT_TEXT, text_input);
			std::vector<std::string> lines;
			pfc::splitStringByLinesFunc([&lines](const pfc::string_part_ref& part){lines.push_back(std::string(part.m_ptr, part.m_ptr + part.m_len)); }, text_input);

			// For each line in text_input, extract an (artist, title) pair.
			std::vector<std::pair<std::string, std::string>> results(lines.size());

			for(size_t line_index = 0; line_index < lines.size(); ++line_index)
			{
				const auto& line = lines[line_index];

				if (std::regex_match(line, artist_match, artist_regex))
				{
					// The first sub_match is the whole string; the next
					// sub_match is the first parenthesized expression.
					if (artist_match.size() == 2)
					{
						std::string artist = artist_match[1].str();

						if (std::regex_match(line, title_match, title_regex))
						{
							// The first sub_match is the whole string; the next
							// sub_match is the first parenthesized expression.
							if (title_match.size() == 2)
							{
								std::string title = title_match[1].str();
								results[line_index] = std::make_pair(artist, title);
							}
							else
							{
								console::print(pfc::string8((line + ": wrong number of title matches (" + to_string(title_match.size()) + ").").c_str()));
							}
						}
						else
						{
							console::print(pfc::string8((line + ": title regex doesn't match.").c_str()));
						}
					}
					else
					{
						console::print(pfc::string8((line + ": wrong number of artist matches (" + to_string(artist_match.size()) + ").").c_str()));
					}
				}
				else
				{
					console::print(pfc::string8((line + ": artist regex doesn't match.").c_str()));
				}
			}

			// Create another vector to hold the actual best track results.
			std::vector<metadb_handle_ptr> best_versions(results.size());

			pfc::list_t<metadb_handle_ptr> library;
			static_api_ptr_t<library_manager> lm;
			lm->get_all_items(library);

			// Lock the database for the duration of this scope.
			DatabaseScopeLock databaseLock;

			for(size_t match_index = 0; match_index < results.size(); ++match_index)
			{
				const auto& match = results[match_index];

				if(match.first.empty() || match.second.empty())
				{
					continue;
				}

				// Create a copy of the library with only tracks by this aritist with a similar title.
				pfc::list_t<metadb_handle_ptr> possible_tracks = library;
				filterTracksByArtist(match.first, possible_tracks);
				filterTracksByCloseTitle(match.second, possible_tracks);

				// Pick the best version of all these tracks and add it to the list if found.
				metadb_handle_ptr track = getBestTrackByTitle(match.second, possible_tracks);

				best_versions[match_index] = track;
			}

			pfc::string8 text_output;
			for(size_t match_index = 0; match_index < results.size(); ++match_index)
			{
				const auto& match = results[match_index];
				if(match.first.empty() || match.second.empty())
				{
					text_output += "Regex problem.";
				}
				else
				{
					text_output += pfc::string8((match.first + " : " + match.second + " : ").c_str());
					if(best_versions[match_index] != 0)
					{
						text_output += best_versions[match_index]->get_path();
					}
					else
					{
						text_output += "No best track found.";
					}
				}
				text_output += "\r\n";
			}

			console::print("complete");
			console::print(text_output);
			uSetDlgItemText(*this, IDC_OUTPUT_TEXT, text_output);
		}
		catch(std::regex_error&)
		{
			console::print("regex error");
			return;
		}
	}
};

void showTextToPlaylistDialogue()
{
	try {
		// ImplementModelessTracking registers our dialog to receive dialog messages thru main app loop's IsDialogMessage().
		// CWindowAutoLifetime creates the window in the constructor (taking the parent window as a parameter) and deletes the object when the window has been destroyed (through WTL's OnFinalMessage).
		new CWindowAutoLifetime<ImplementModelessTracking<TextToPlaylistDialogue> >(core_api::get_main_window());
	} catch(std::exception const & e) {
		popup_message::g_complain("Dialog creation failure", e);
	}
}

} // namespace bestversion
