#include "TextToPlaylistDialogue.h"
#include "FoobarSDKWrapper.h"
#include "ATLHelpersWrapper.h"
#include "BestVersion.h"
#include "DatabaseScopeLock.h"
#include "resource.h"
#include "ToString.h"
#include "PlaylistGenerator.h"

#include <memory>
#include <regex>

namespace bestversion
{

class TextToPlaylistDialogue : public CDialogImpl<TextToPlaylistDialogue> {
public:
	TextToPlaylistDialogue()
		: CDialogImpl<TextToPlaylistDialogue>()
		, m_best_versions()
	{}

	enum { IDD = IDD_TEXT_TO_PLAYLIST_DIALOGUE };

	BEGIN_MSG_MAP(TextToPlaylistDialogue)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_REGEX_TEXT, EN_CHANGE, OnRegexChange)
		COMMAND_HANDLER_EX(IDC_ARTIST_INDEX_TEXT, EN_CHANGE, OnIndexChange)
		COMMAND_HANDLER_EX(IDC_TITLE_INDEX_TEXT, EN_CHANGE, OnIndexChange)
		COMMAND_HANDLER_EX(IDC_INPUT_TEXT, EN_CHANGE, OnTextInputChange)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOk)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		SetDlgItemText(IDC_REGEX_TEXT,	_T("[^/]*/([^/]+)/[^/]+/(?:[^/]+/)?[\\d\\s-]*([^/]+)\\.(?:mp3|flac|ogg)"));
		SetDlgItemText(IDC_ARTIST_INDEX_TEXT, _T("0"));
		SetDlgItemText(IDC_TITLE_INDEX_TEXT, _T("1"));
		ShowWindowCentered(*this, GetParent()); // Function declared in SDK helpers.
		return TRUE;
	}

	void OnRegexChange(UINT, int, CWindow)
	{
		handle_regex_changed();
	}

	void OnIndexChange(UINT, int, CWindow)
	{
		handle_regex_changed();
	}

	void OnTextInputChange(UINT, int, CWindow)
	{
		handle_input_text_changed();
	}

	void OnOk(UINT, int, CWindow)
	{
		if(!m_best_versions.empty())
		{
			// todo: allow user to name playlist.
			// Remove non-existent tracks then create a playlist containing them.
			m_best_versions.erase(std::remove_if(std::begin(m_best_versions), std::end(m_best_versions), [](const metadb_handle_ptr& ptr){ return ptr == 0; }), std::end(m_best_versions));
			generatePlaylistFromTracks(m_best_versions);
		}

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

		pfc::string8 regex_string;
		uGetDlgItemText(*this, IDC_REGEX_TEXT, regex_string);
		pfc::string8 artist_index_string;
		uGetDlgItemText(*this, IDC_ARTIST_INDEX_TEXT, artist_index_string);
		pfc::string8 title_index_string;
		uGetDlgItemText(*this, IDC_TITLE_INDEX_TEXT, title_index_string);

		// Work out indices.
		const int artist_index = from_string<int>(std::string(artist_index_string));
		const int title_index = from_string<int>(std::string(title_index_string));

		if(artist_index < 0)
		{
			const std::string error_message = std::string("Artist index is invalid (") + artist_index_string.get_ptr() + ").";
			console::print(error_message.c_str());
			uSetDlgItemText(*this, IDC_OUTPUT_TEXT, error_message.c_str());
			return;
		}

		if(title_index < 0)
		{
			const std::string error_message = std::string("Title index is invalid (") + title_index_string.get_ptr() + ").";
			console::print(error_message.c_str());
			uSetDlgItemText(*this, IDC_OUTPUT_TEXT, error_message.c_str());
			return;
		}

		const int highest_index = std::max(artist_index, title_index);

		std::unique_ptr<std::regex> regex;

		// Compile regexes.
		try
		{
			regex = std::unique_ptr<std::regex>(new std::regex(regex_string));
		}
		catch(std::regex_error& e)
		{
			// Todo: issue more information.
			const std::string error_message = std::string("Regex error: ") + e.what();
			console::print(error_message.c_str());
			uSetDlgItemText(*this, IDC_OUTPUT_TEXT, error_message.c_str());
			return;
		}

		if(!regex)
		{
			return;
		}

		std::smatch regex_match;

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

			if (std::regex_match(line, regex_match, *regex))
			{
				// The first match will be the full string.
				// The rest are the sub-matches which we want.
				// Check we have enough to index into.
				if (regex_match.size() >= static_cast<size_t>(highest_index + 1))
				{
					const std::string& artist = regex_match[artist_index + 1].str();
					const std::string& title = regex_match[title_index + 1].str();
					results[line_index] = std::make_pair(artist, title);
				}
				else
				{
					console::print(pfc::string8((line + ": Not enough regex matches (" + to_string(regex_match.size()) + ").").c_str()));
				}
			}
			else
			{
				console::print(pfc::string8((line + ": Regex doesn't match.").c_str()));
			}
		}

		// Create another vector to hold the actual best track results.
		m_best_versions.clear();
		m_best_versions.resize(results.size());

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

			m_best_versions[match_index] = track;
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
				if(m_best_versions[match_index] != 0)
				{
					text_output += m_best_versions[match_index]->get_path();
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

	std::vector<metadb_handle_ptr> m_best_versions;
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
