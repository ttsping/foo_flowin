#include "stdafx.h"

#ifdef FOOBAR2000_DESKTOP_WINDOWS

#include "window_placement_helper.h"

static bool g_is_enabled()
{
	return standard_config_objects::query_remember_window_positions();
}

static BOOL CALLBACK __MonitorEnumProc(
  HMONITOR hMonitor,  // handle to display monitor
  HDC hdcMonitor,     // handle to monitor DC
  LPRECT lprcMonitor, // monitor intersection rectangle
  LPARAM dwData       // data
  ) {
	RECT * clip = (RECT*)dwData;
	RECT newclip;
	if (UnionRect(&newclip,clip,lprcMonitor)) {
		*clip = newclip;
	}
	return TRUE;
}

static bool test_rect(const RECT * rc) {
	RECT clip = {};
	if (EnumDisplayMonitors(NULL,NULL,__MonitorEnumProc,(LPARAM)&clip)) {
		const LONG sanitycheck = 4;
		const LONG cwidth = clip.right - clip.left;
		const LONG cheight = clip.bottom - clip.top;
		
		const LONG width = rc->right - rc->left;
		const LONG height = rc->bottom - rc->top;

		if (width > cwidth * sanitycheck || height > cheight * sanitycheck) return false;
	}
	
	return MonitorFromRect(rc,MONITOR_DEFAULTTONULL) != NULL;
}


bool cfg_window_placement_common::read_from_window(HWND window)
{
	WINDOWPLACEMENT wp = {};
	if (g_is_enabled()) {
		wp.length = sizeof(wp);
		if (!GetWindowPlacement(window,&wp)) {
			PFC_ASSERT(!"GetWindowPlacement fail!");
			memset(&wp,0,sizeof(wp));
		} else {
			// bad, breaks with taskbar on top
			/*if (wp.showCmd == SW_SHOWNORMAL) {
				GetWindowRect(window, &wp.rcNormalPosition);
			}*/

			if ( !IsWindowVisible( window ) ) wp.showCmd = SW_HIDE;
		}
		/*else
		{
			if (!IsWindowVisible(window)) wp.showCmd = SW_HIDE;
		}*/
	}
	m_data = wp;
	return m_data.length == sizeof(m_data);
}

bool cfg_window_placement_common::apply_to_window(HWND window, bool allowHidden) {
	bool ret = false;
	if (g_is_enabled())
	{
		if (m_data.length == sizeof(m_data) && test_rect(&m_data.rcNormalPosition))
		{
			if (allowHidden || m_data.showCmd != SW_HIDE) {
				if (m_data.showCmd == SW_HIDE && (m_data.flags & WPF_RESTORETOMAXIMIZED)) {
					// Special case of hidden-from-maximized
					auto fix = m_data;
					fix.showCmd = SW_SHOWMINIMIZED;
					if (SetWindowPlacement(window, &fix)) {
						ShowWindow(window, SW_HIDE);
						ret = true;
					}
				} else {
					if (SetWindowPlacement(window, &m_data)) {
						ret = true;
					}
				}
			}
		}
	}

	return ret;
}

void cfg_window_placement::on_window_creation_silent(HWND window) {
	PFC_ASSERT(!m_windows.have_item(window));
	m_windows.add_item(window);
}
bool cfg_window_placement::on_window_creation(HWND window, bool allowHidden) {
	
	PFC_ASSERT(!m_windows.have_item(window));
	m_windows.add_item(window);
	return apply_to_window(window, allowHidden);
}


void cfg_window_placement::on_window_destruction(HWND window)
{
	if (m_windows.have_item(window))
	{
		read_from_window(window);
		m_windows.remove_item(window);
	}
}

void cfg_window_placement_common::get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
	if (m_data.length == sizeof(m_data)) {
		p_stream->write_object(&m_data, sizeof(m_data), p_abort);
	}
}

void cfg_window_placement::get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
	if (g_is_enabled()) {
		{
			t_size n, m = m_windows.get_count();
			for(n=0;n<m;n++) {
				HWND window = m_windows[n];
				PFC_ASSERT(IsWindow(window));
				if (IsWindow(window) && read_from_window(window)) break;
			}
		}

		cfg_window_placement_common::get_data_raw(p_stream, p_abort);
	}
}

void cfg_window_placement_common::set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
	WINDOWPLACEMENT temp;
	if (p_sizehint == sizeof(temp)) {
		p_stream->read_object(&temp, sizeof(temp), p_abort);
		if (temp.length == sizeof(temp)) m_data = temp;
	}
}


cfg_window_size::cfg_window_size(const GUID & p_guid) : cfg_var(p_guid), m_width(~0), m_height(~0) {}

static BOOL SetWindowSize(HWND p_wnd,unsigned p_x,unsigned p_y)
{
	if (p_x != ~0 && p_y != ~0)
		return SetWindowPos(p_wnd,0,0,0,p_x,p_y,SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
	else
		return FALSE;
}

bool cfg_window_size::on_window_creation(HWND p_wnd)
{
	bool ret = false;
	PFC_ASSERT(!m_windows.have_item(p_wnd));
	m_windows.add_item(p_wnd);

	if (g_is_enabled())
	{
		if (SetWindowSize(p_wnd,m_width,m_height)) ret = true;
	}

	return ret;
}

void cfg_window_size::on_window_destruction(HWND p_wnd)
{
	if (m_windows.have_item(p_wnd))
	{
		read_from_window(p_wnd);
		m_windows.remove_item(p_wnd);
	}
}

bool cfg_window_size::read_from_window(HWND p_wnd)
{
	if (g_is_enabled())
	{
		RECT r;
		if (GetWindowRect(p_wnd,&r))
		{
			m_width = r.right - r.left;
			m_height = r.bottom - r.top;
			return true;
		}
		else
		{
			m_width = m_height = ~0;
			return false;
		}
	}
	else
	{
		m_width = m_height = ~0;
		return false;
	}
}

void cfg_window_size::get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
	if (g_is_enabled())  {
		{
			t_size n, m = m_windows.get_count();
			for(n=0;n<m;n++) {
				HWND window = m_windows[n];
				PFC_ASSERT(IsWindow(window));
				if (IsWindow(window) && read_from_window(window)) break;
			}
		}

		if (m_width != ~0 && m_height != ~0) {
			p_stream->write_lendian_t(m_width,p_abort);
			p_stream->write_lendian_t(m_height,p_abort);
		}
	}
}

void cfg_window_size::set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
	if (p_sizehint == 0) return;
	t_uint32 width,height;
	try {
		p_stream->read_lendian_t(width,p_abort);
		p_stream->read_lendian_t(height,p_abort);
	} catch(exception_io_data) {return;}

	m_width = width; m_height = height;
}
#endif // FOOBAR2000_DESKTOP_WINDOWS
