#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <ibus.h>
#include <stdio.h>
#include <string.h>
#include "unikey.h"
#include "vnconv.h"


const gchar          *Unikey_IMNames[]    = {"Telex", "Vni"};
const UkInputMethod   Unikey_IM[]         = {UkTelex, UkVni};
const unsigned int    NUM_INPUTMETHOD     = sizeof(Unikey_IMNames)/sizeof(Unikey_IMNames[0]);

const gchar          *Unikey_OCNames[]    = {"Unicode",           "TCVN3",            "VNI Win",              "VIQR"};
const unsigned int    Unikey_OC[]         = {CONV_CHARSET_XUTF8,  CONV_CHARSET_TCVN3, CONV_CHARSET_VNIWIN,    CONV_CHARSET_VIQR};
const unsigned int    NUM_OUTPUTCHARSET   = sizeof(Unikey_OCNames)/sizeof(Unikey_OCNames[0]);

typedef struct _IBusUnikeyEngine          IBusUnikeyEngine;
typedef struct _IBusUnikeyEngineClass     IBusUnikeyEngineClass;

struct _IBusUnikeyEngine
{
	IBusEngine parent;

	/* members */
	IBusPropList    *prop_list;

	UkInputMethod   im; // input method
	unsigned int    oc; // output charset
	gchar           preeditstr[128]; // len = MAX_UK_ENGINE in ukengine.h
};

struct _IBusUnikeyEngineClass {
	IBusEngineClass parent;
};

// prototype
static void ibus_unikey_engine_class_init(IBusUnikeyEngineClass *kclass);
static void ibus_unikey_engine_init(IBusUnikeyEngine *unikey);

static GObject *ibus_unikey_engine_constructor(GType type,
											   guint n_construct_params,
											   GObjectConstructParam *construct_params);

static void ibus_unikey_engine_destroy(IBusUnikeyEngine *unikey);
static gboolean ibus_unikey_engine_process_key_event(IBusEngine *engine,
													 guint keyval,
													 guint modifiers);

static void ibus_unikey_engine_focus_in(IBusEngine *engine);
static void ibus_unikey_engine_focus_out(IBusEngine *engine);
static void ibus_unikey_engine_reset(IBusEngine *engine);
static void ibus_unikey_engine_enable(IBusEngine *engine);
static void ibus_unikey_engine_disable(IBusEngine *engine);
static void ibus_unikey_engine_property_activate(IBusEngine *engine,
												 const gchar *prop_name,
												 guint prop_state);

static gboolean ibus_unikey_engine_process_key_event_preedit(IBusEngine *engine,
															 guint keyval,
															 guint modifiers);


static IBusEngineClass *parent_class = NULL;
static IBusConfig      *config       = NULL;

GType ibus_unikey_engine_get_type(void)
{
	static GType type = 0;

	static const GTypeInfo type_info = {
		sizeof(IBusUnikeyEngineClass),
		(GBaseInitFunc)NULL,
		(GBaseFinalizeFunc)NULL,
		(GClassInitFunc)ibus_unikey_engine_class_init,
		NULL,
		NULL,
		sizeof(IBusUnikeyEngine),
		0,
		(GInstanceInitFunc)ibus_unikey_engine_init,
	};

	if (type == 0)
	{
		type = g_type_register_static (IBUS_TYPE_ENGINE,
									   "IBusUnikeyEngine",
									   &type_info,
									   (GTypeFlags)0);
	}

	return type;
}

void ibus_unikey_init(IBusBus *bus)
{
	UnikeySetup();
	config = ibus_bus_get_config(bus);
}

void ibus_unikey_exit()
{
	UnikeyCleanup();
}

static void ibus_unikey_engine_class_init(IBusUnikeyEngineClass *klass)
{
#ifdef DEBUG
	ibus_warning("class_init()");
#endif

	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS(klass);
	IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);

	parent_class = (IBusEngineClass *)g_type_class_peek_parent(klass);

	object_class->constructor = ibus_unikey_engine_constructor;
	ibus_object_class->destroy = (IBusObjectDestroyFunc)ibus_unikey_engine_destroy;

	engine_class->process_key_event = ibus_unikey_engine_process_key_event;

	engine_class->reset = ibus_unikey_engine_reset;
	engine_class->enable = ibus_unikey_engine_enable;
	engine_class->disable = ibus_unikey_engine_disable;

	engine_class->focus_in = ibus_unikey_engine_focus_in;
	engine_class->focus_out = ibus_unikey_engine_focus_out;
	engine_class->property_activate = ibus_unikey_engine_property_activate;
}

static void ibus_unikey_engine_init(IBusUnikeyEngine *unikey)
{
#ifdef DEBUG
	ibus_warning("init()");
#endif
}

static GObject *ibus_unikey_engine_constructor(GType type,
											   guint n_construct_params,
											   GObjectConstructParam *construct_params)
{
#ifdef DEBUG
	ibus_warning("constructor()");
#endif

	IBusUnikeyEngine *unikey;
	const gchar *engine_name;

	unikey = (IBusUnikeyEngine*)G_OBJECT_CLASS(parent_class)->constructor(type, n_construct_params, construct_params);

	engine_name = ibus_engine_get_name((IBusEngine*)unikey);

	return (GObject*)unikey;
}

static void ibus_unikey_engine_destroy(IBusUnikeyEngine *unikey)
{
}

static gboolean ibus_unikey_engine_process_key_event(IBusEngine *engine,
													 guint keyval,
													 guint modifiers)
{
#ifdef DEBUG
	ibus_warning("process_key_event(%d, %d)", keyval, modifiers);
#endif

	return ibus_unikey_engine_process_key_event_preedit(engine, keyval, modifiers);
}

static void ibus_unikey_engine_focus_in(IBusEngine *engine)
{
#ifdef DEBUG
	ibus_warning("focus_in()");
#endif

	UnikeySetInputMethod(((IBusUnikeyEngine*)engine)->im);
	UnikeySetOutputCharset(((IBusUnikeyEngine*)engine)->oc);
}

static void ibus_unikey_engine_focus_out(IBusEngine *engine)
{
#ifdef DEBUG
	ibus_warning("focus_out()");
#endif

	ibus_unikey_engine_reset(engine);
}

static void ibus_unikey_engine_reset(IBusEngine *engine)
{
#ifdef DEBUG
	ibus_warning("reset()");
#endif

	UnikeyResetBuf();
}

static void ibus_unikey_engine_enable(IBusEngine *engine)
{
#ifdef DEBUG
	ibus_warning("enable()");
#endif
}

static void ibus_unikey_engine_disable(IBusEngine *engine)
{
#ifdef DEBUG
	ibus_warning("disable()");
#endif
}

static void ibus_unikey_engine_property_activate(IBusEngine *engine,
												 const gchar *prop_name,
												 guint prop_state)
{


}

static gboolean ibus_unikey_engine_process_key_event_preedit(IBusEngine *engine,
															 guint keyval,
															 guint modifiers)
{

	

	return FALSE;
}


// utils
IBusComponent *ibus_unikey_get_component()
{
	IBusComponent *component;

	component = ibus_component_new("org.freedesktop.IBus.Unikey",
								   "Unikey",
								   PACKAGE_VERSION,
								   "GPL",
								   "Le Quoc Tuan <mr.lequoctuan@gmail.com>",
								   PACKAGE_BUGREPORT,
								   "",
								   PACKAGE_NAME);

	ibus_component_add_engine(component,
							  ibus_engine_desc_new("Unikey",
												   "Unikey",
												   "Unikey Input Method",
												   "vi",
												   "GPL",
												   "Le Quoc Tuan <mr.lequoctuan@gmail.com>",
												   PKGDATADIR"/icons/ibus-unikey.png",
												   "us"));

	return component;
}
