#include <stdio.h>
#include <stdlib.h>

#include "file_list.h"
//#include "gui.h"

// Externals

extern Config *g_config;

static char s_LastDir[128] = "/";
int RunFileBrowser(char *source, char *outname, const char *types[],
		const char *info) {

	int size = 0;
	int index;
	int offset_start, offset_end;
	static int max_entries = 8;

	static int spy;
	int y, i;

	// Create file list
	FileList *list = new FileList(source ? source : s_LastDir, types);
	if (list == NULL)
		return 0;

	RESTART:

	spy = 74;

	size = list->Size();

	index = 0;
	offset_start = 0;
	offset_end = size > max_entries ? max_entries : size;

	g_dirty = 1;
	while (1) {
		// Get time and battery every second
		if (update_time()) {
			update_battery();
			g_dirty = 1;
		}

		// Parse input
		readkey();
		// TODO - put exit keys

		// Go to previous folder or return ...
		if (parsekey(DINGOO_B) || parsekey(DINGOO_LEFT)) {
			list->Enter(-1);
			goto RESTART;
		}

		// Enter folder or select rom ...
		if (parsekey(DINGOO_A) || parsekey(DINGOO_RIGHT)) {
			if (list->GetSize(index) == -1) {
				list->Enter(index);
				goto RESTART;
			} else {
				strncpy(outname, list->GetPath(index), 128);
				break;
			}
		}

		if (parsekey(DINGOO_X)) {
			return 0;
		}

		if (size > 0) {
			// Move through file list
			if (parsekey(DINGOO_UP, 1)) {
					if (index > offset_start){
						index--;
						spy -= 15;
					} else if (offset_start > 0) {
						index--;
						offset_start--;
						offset_end--;
					} else {
						index = size - 1;
						offset_end = size;
						offset_start = size <= max_entries ? 0 : offset_end - max_entries;
						spy = 74 + 15*(index - offset_start);
					}
			}

			if (parsekey(DINGOO_DOWN, 1)) {
				if (index < offset_end - 1){
					index++;
					spy += 15;
				} else if (offset_end < size) {
					index++;
					offset_start++;
					offset_end++;
				} else {
					index = 0;
					offset_start = 0;
					offset_end = size <= max_entries ? size : max_entries;
					spy = 74;
				}
			}

			if (parsekey(DINGOO_L, 1)) {
				if (index > offset_start) {
					index = offset_start;

					spy = 74;

				} else if (index - max_entries >= 0){
						index -= max_entries;
						offset_start -= max_entries;
						offset_end = offset_start + max_entries;
				} else
					goto RESTART;
			}

			if (parsekey(DINGOO_R, 1)) {
				if (index < offset_end-1) {
					index = offset_end-1;

					spy = 74 + 15*(index-offset_start);

				} else if (offset_end + max_entries <= size) {
						index += max_entries;
						offset_end += max_entries;
						offset_start += max_entries;
				} else {
					index = size - 1;
					spy = 74 + 15*(max_entries-1);
					offset_end = size;
					offset_start = offset_end - max_entries;
				}
			}
		}

		// Draw stuff
		if (g_dirty) {
			draw_bg(vbuffer, g_bg);

			// Draw time and battery every minute
			DrawText(vbuffer, g_time, 148, 5);
			DrawText(vbuffer, g_battery, 214, 5);

			DrawChar(vbuffer, SP_BROWSER, 40, 38);

			// Draw file list
			for (i = offset_start, y = 70; i < offset_end; i++, y += 15) {
				DrawText(vbuffer, list->GetName(i), 36, y);
			}

			// Draw info
			if (info)
				DrawText(vbuffer, info, 16, 225);
			else {
				if (list->GetSize(index) == -1)
					DrawText(vbuffer, "Open folder?", 16, 225);
				else
					DrawText(vbuffer, "Open file?", 16, 225);
			}

			// Draw selector
			DrawChar(vbuffer, SP_SELECTOR, 20, spy);

			// Draw offset marks
			if (offset_start > 0)
				DrawChar(vbuffer, SP_UPARROW, 274, 62);
			if (offset_end < list->Size())
				DrawChar(vbuffer, SP_DOWNARROW, 274, 203);

			g_dirty = 0;
		}

		dingoo_timer_delay(16);

		// Update real screen
		memcpy(dingoo_screen15, vbuffer, 320 * 240 * sizeof(unsigned short));
	}

	if (source == NULL)
		strncpy(s_LastDir, list->GetCurDir(), 128);
	delete list;

	return 1;
}
