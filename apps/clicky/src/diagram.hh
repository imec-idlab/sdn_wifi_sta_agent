#ifndef CLICKY_DIAGRAM_HH
#define CLICKY_DIAGRAM_HH 1
#include <gtk/gtk.h>
#include <vector>
#include "rectangle.hh"
#include "rectsearch.hh"
#include <click/bitvector.hh>
#include <clicktool/elementt.hh>
class Bitvector;
namespace clicky {
class wmain;
class handler_value;
class dwidget;
class delt;
class dcss_set;

class wdiagram { public:

    wdiagram(wmain *rw);
    ~wdiagram();

    wmain *main() const {
	return _rw;
    }
    
    void router_create(bool incremental, bool always);

    int scale_step() const {
	return _scale_step;
    }
    double scale() const {
	return _scale;
    }

    dcss_set *ccss() const {
	return _css_set;
    }
    String ccss_text() const;
    void set_ccss_text(const String &text);

    delt *elt(const String &name) const {
	return _elt_map[name];
    }
    
    rect_search<dwidget> &rects() {
	return _rects;
    }

    void notify_shadow(double shadow) {
	_elt_expand = std::max(_elt_expand, shadow + 2);
    }
    
    inline void redraw();
    inline void redraw(rectangle r);

    void display(const String &ename, bool scroll_to);
    void zoom(bool incremental, int amount);
    void scroll_recenter(point p);

    void on_expose(const GdkRectangle *r);
    gboolean on_event(GdkEvent *event);

    // handlers
    void notify_read(handler_value *hv);

    point window_to_canvas(double x, double y) const;
    point canvas_to_window(double x, double y) const;
    rectangle canvas_to_window(const rectangle &r) const;

    void export_diagram(const char *filename, bool eps);

    struct reachable_t {
	Bitvector main;
	HashTable<String, Bitvector> compound;
	bool operator()(const String &s, int eindex) const {
	    if (!s)
		return main.size() && main[eindex];
	    const Bitvector *v = compound.get_pointer(s);
	    return v && v->size() && (*v)[eindex];
	}
    };
    const reachable_t &downstream(const String &str);
    const reachable_t &upstream(const String &str);
    
    enum { c_element = 0, c_main = 9, c_hand = 10, ncursors = 11 };
    
  private:

    wmain *_rw;
    GtkWidget *_widget;
    GtkAdjustment *_horiz_adjust;
    GtkAdjustment *_vert_adjust;
    dcss_set *_css_set;
    dcss_set *_base_css_set;
    unsigned _pango_generation;
    double _elt_expand;
    
    int _scale_step;
    double _scale;
    int _origin_x;
    int _origin_y;

    delt *_relt;
    rect_search<dwidget> _rects;
    HashTable<String, delt *> _elt_map;
    bool _layout;

    std::list<delt *> _highlight[3];

    enum {
	drag_none,
	drag_start,
	drag_dragging,
	drag_rect_start,
	drag_rect_dragging,
	drag_hand_start,
	drag_hand_dragging
    };
    
    rectangle _dragr;
    int _drag_state;

    GdkCursor *_cursor[ncursors];

    int _last_cursorno;

    HashTable<String, reachable_t> _downstreams;
    HashTable<String, reachable_t> _upstreams;
    
    void initialize();
    void layout();

    void expose(delt *e, rectangle *expose_rect);
    void unhighlight(uint8_t htype, rectangle *expose_rect);
    void highlight(delt *e, uint8_t htype, rectangle *expose_rect, bool incremental);

    delt *point_elt(const point &p) const;
    void set_cursor(delt *e, double x, double y, int state);
    point scroll_center() const;
    inline void find_rect_elts(const rectangle &r, std::vector<dwidget *> &result) const;
    void on_drag_motion(const point &p);
    void on_drag_rect_motion(const point &p);
    void on_drag_hand_motion(double x_root, double y_root);
    void on_drag_complete();
    void on_drag_rect_complete();

    struct reachable_match_t {
	String _name;
	int _port;
	bool _forward;
	RouterT *_router;
	String _router_name;
	ProcessingT *_processing;
	Bitvector _seed;

	reachable_match_t(const String &name, int port,
			  bool forward, RouterT *router,
			  ProcessingT *processing);
	reachable_match_t(const reachable_match_t &m, ElementT *subelement);
	~reachable_match_t();
	inline bool get_seed(int eindex, int port) const;
	inline void set_seed(const ConnectionT &conn);
	inline void set_seed_connections(ElementT *element, int port);
	bool add_matches(reachable_t &reach);
	void export_matches(reachable_t &reach);
    };
    void calculate_reachable(const String &str, bool forward, reachable_t &reach);
    
    friend class delt;
    
};


inline void wdiagram::redraw()
{
    gtk_widget_queue_draw(_widget);
}

inline void wdiagram::redraw(rectangle r)
{
    r.expand(_elt_expand);
    r.scale(_scale);
    r.integer_align();
    gtk_widget_queue_draw_area(_widget, (gint) (r.x() - _horiz_adjust->value - _origin_x), (gint) (r.y() - _vert_adjust->value - _origin_y), (gint) r.width(), (gint) r.height());
}

inline point wdiagram::window_to_canvas(double x, double y) const
{
    return point((x + _origin_x) / _scale, (y + _origin_y) / _scale);
}

inline point wdiagram::canvas_to_window(double x, double y) const
{
    return point(x * _scale - _origin_x, y * _scale - _origin_y);
}

inline rectangle wdiagram::canvas_to_window(const rectangle &r) const
{
    return rectangle(r.x() * _scale - _origin_x, r.y() * _scale - _origin_y,
		     r.width() * _scale, r.height() * _scale);
}

inline point wdiagram::scroll_center() const
{
    return window_to_canvas(_horiz_adjust->value + _horiz_adjust->page_size / 2,
			    _vert_adjust->value + _vert_adjust->page_size / 2);
}

inline const wdiagram::reachable_t &wdiagram::upstream(const String &str)
{
    reachable_t &r = _upstreams[str];
    if (!r.main.size())
	calculate_reachable(str, false, r);
    return r;
}

inline const wdiagram::reachable_t &wdiagram::downstream(const String &str)
{
    reachable_t &r = _downstreams[str];
    if (!r.main.size())
	calculate_reachable(str, true, r);
    return r;
}

}
#endif
