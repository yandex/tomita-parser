/* The following code is the modified part of the libxslt
 * available at http://xmlsoft.org/libxslt
 * under the terms of the MIT License
 * http://opensource.org/licenses/mit-license.html
 */

/*это выдранные куски из библиотеки htmlparser-a из libxml, нужные 
для функций xsltSaveResultTo... Так как htmlparser из libxml вырезан by leo, то от
библиотеки осталось только несколько необходимых функций и определений*/

#include <HTMLparser.h>
#include <HTMLtree.h>

void
htmlDocContentDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr cur,
	                 const char *encoding) {
    htmlDocContentDumpFormatOutput(buf, cur, encoding, 1);
}


#define FONTSTYLE "tt", "i", "b", "u", "s", "strike", "big", "small"
#define NB_FONTSTYLE 8
#define PHRASE "em", "strong", "dfn", "code", "samp", "kbd", "var", "cite", "abbr", "acronym"
#define NB_PHRASE 10
#define SPECIAL "a", "img", "applet", "embed", "object", "font", "basefont", "br", "script", "map", "q", "sub", "sup", "span", "bdo", "iframe"
#define NB_SPECIAL 16
#define INLINE PCDATA FONTSTYLE PHRASE SPECIAL FORMCTRL
#define NB_INLINE NB_PCDATA + NB_FONTSTYLE + NB_PHRASE + NB_SPECIAL + NB_FORMCTRL
#define BLOCK HEADING, LIST "pre", "p", "dl", "div", "center", "noscript", "noframes", "blockquote", "form", "isindex", "hr", "table", "fieldset", "address"
#define NB_BLOCK NB_HEADING + NB_LIST + 14
#define FORMCTRL "input", "select", "textarea", "label", "button"
#define NB_FORMCTRL 5
#define PCDATA
#define NB_PCDATA 0
#define HEADING "h1", "h2", "h3", "h4", "h5", "h6"
#define NB_HEADING 6
#define LIST "ul", "ol", "dir", "menu"
#define NB_LIST 4
#define MODIFIER
#define NB_MODIFIER 0
#define FLOW BLOCK,INLINE
#define NB_FLOW NB_BLOCK + NB_INLINE
#define EMPTY NULL



static const char* const html_flow[] = { FLOW, NULL } ;
static const char* const html_inline[] = { INLINE, NULL } ;

/* placeholders: elts with content but no subelements */
static const char* const html_pcdata[] = { NULL } ;
#define html_cdata html_pcdata


#define COREATTRS "id", "class", "style", "title"
#define NB_COREATTRS 4
#define I18N "lang", "dir"
#define NB_I18N 2
#define EVENTS "onclick", "ondblclick", "onmousedown", "onmouseup", "onmouseover", "onmouseout", "onkeypress", "onkeydown", "onkeyup"
#define NB_EVENTS 9
#define ATTRS COREATTRS,I18N,EVENTS
#define NB_ATTRS NB_NB_COREATTRS + NB_I18N + NB_EVENTS
#define CELLHALIGN "align", "char", "charoff"
#define NB_CELLHALIGN 3
#define CELLVALIGN "valign"
#define NB_CELLVALIGN 1

static const char* const html_attrs[] = { ATTRS, NULL } ;
static const char* const core_i18n_attrs[] = { COREATTRS, I18N, NULL } ;
static const char* const core_attrs[] = { COREATTRS, NULL } ;
static const char* const i18n_attrs[] = { I18N, NULL } ;



/* Other declarations that should go inline ... */
static const char* const a_attrs[] = { ATTRS, "charset", "type", "name",
	"href", "hreflang", "rel", "rev", "accesskey", "shape", "coords",
	"tabindex", "onfocus", "onblur", NULL } ;
static const char* const target_attr[] = { "target", NULL } ;
static const char* const rows_cols_attr[] = { "rows", "cols", NULL } ;
static const char* const alt_attr[] = { "alt", NULL } ;
static const char* const src_alt_attrs[] = { "src", "alt", NULL } ;
static const char* const href_attrs[] = { "href", NULL } ;
static const char* const clear_attrs[] = { "clear", NULL } ;
static const char* const inline_p[] = { INLINE, "p", NULL } ;

static const char* const flow_param[] = { FLOW, "param", NULL } ;
static const char* const applet_attrs[] = { COREATTRS , "codebase",
		"archive", "alt", "name", "height", "width", "align",
		"hspace", "vspace", NULL } ;
static const char* const area_attrs[] = { "shape", "coords", "href", "nohref",
	"tabindex", "accesskey", "onfocus", "onblur", NULL } ;
static const char* const basefont_attrs[] =
	{ "id", "size", "color", "face", NULL } ;
static const char* const quote_attrs[] = { ATTRS, "cite", NULL } ;
static const char* const body_contents[] = { FLOW, "ins", "del", NULL } ;
static const char* const body_attrs[] = { ATTRS, "onload", "onunload", NULL } ;
static const char* const body_depr[] = { "background", "bgcolor", "text",
	"link", "vlink", "alink", NULL } ;
static const char* const button_attrs[] = { ATTRS, "name", "value", "type",
	"disabled", "tabindex", "accesskey", "onfocus", "onblur", NULL } ;


static const char* const col_attrs[] = { ATTRS, "span", "width", CELLHALIGN, CELLVALIGN, NULL } ;
static const char* const col_elt[] = { "col", NULL } ;
static const char* const edit_attrs[] = { ATTRS, "datetime", "cite", NULL } ;
static const char* const compact_attrs[] = { ATTRS, "compact", NULL } ;
static const char* const dl_contents[] = { "dt", "dd", NULL } ;
static const char* const compact_attr[] = { "compact", NULL } ;
static const char* const label_attr[] = { "label", NULL } ;
static const char* const fieldset_contents[] = { FLOW, "legend" } ;
static const char* const font_attrs[] = { COREATTRS, I18N, "size", "color", "face" , NULL } ;
static const char* const form_contents[] = { HEADING, LIST, INLINE, "pre", "p", "div", "center", "noscript", "noframes", "blockquote", "isindex", "hr", "table", "fieldset", "address", NULL } ;
static const char* const form_attrs[] = { ATTRS, "method", "enctype", "accept", "name", "onsubmit", "onreset", "accept-charset", NULL } ;
static const char* const frame_attrs[] = { COREATTRS, "longdesc", "name", "src", "frameborder", "marginwidth", "marginheight", "noresize", "scrolling" , NULL } ;
static const char* const frameset_attrs[] = { COREATTRS, "rows", "cols", "onload", "onunload", NULL } ;
static const char* const frameset_contents[] = { "frameset", "frame", "noframes", NULL } ;
static const char* const head_attrs[] = { I18N, "profile", NULL } ;
static const char* const head_contents[] = { "title", "isindex", "base", "script", "style", "meta", "link", "object", NULL } ;
static const char* const hr_depr[] = { "align", "noshade", "size", "width", NULL } ;
static const char* const version_attr[] = { "version", NULL } ;
static const char* const html_content[] = { "head", "body", "frameset", NULL } ;
static const char* const iframe_attrs[] = { COREATTRS, "longdesc", "name", "src", "frameborder", "marginwidth", "marginheight", "scrolling", "align", "height", "width", NULL } ;
static const char* const img_attrs[] = { ATTRS, "longdesc", "name", "height", "width", "usemap", "ismap", NULL } ;
static const char* const embed_attrs[] = { COREATTRS, "align", "alt", "border", "code", "codebase", "frameborder", "height", "hidden", "hspace", "name", "palette", "pluginspace", "pluginurl", "src", "type", "units", "vspace", "width", NULL } ;
static const char* const input_attrs[] = { ATTRS, "type", "name", "value", "checked", "disabled", "readonly", "size", "maxlength", "src", "alt", "usemap", "ismap", "tabindex", "accesskey", "onfocus", "onblur", "onselect", "onchange", "accept", NULL } ;
static const char* const prompt_attrs[] = { COREATTRS, I18N, "prompt", NULL } ;
static const char* const label_attrs[] = { ATTRS, "for", "accesskey", "onfocus", "onblur", NULL } ;
static const char* const legend_attrs[] = { ATTRS, "accesskey", NULL } ;
static const char* const align_attr[] = { "align", NULL } ;
static const char* const link_attrs[] = { ATTRS, "charset", "href", "hreflang", "type", "rel", "rev", "media", NULL } ;
static const char* const map_contents[] = { BLOCK, "area", NULL } ;
static const char* const name_attr[] = { "name", NULL } ;
static const char* const action_attr[] = { "action", NULL } ;
static const char* const blockli_elt[] = { BLOCK, "li", NULL } ;
static const char* const meta_attrs[] = { I18N, "http-equiv", "name", "scheme", NULL } ;
static const char* const content_attr[] = { "content", NULL } ;
static const char* const type_attr[] = { "type", NULL } ;
static const char* const noframes_content[] = { "body", FLOW MODIFIER, NULL } ;
static const char* const object_contents[] = { FLOW, "param", NULL } ;
static const char* const object_attrs[] = { ATTRS, "declare", "classid", "codebase", "data", "type", "codetype", "archive", "standby", "height", "width", "usemap", "name", "tabindex", NULL } ;
static const char* const object_depr[] = { "align", "border", "hspace", "vspace", NULL } ;
static const char* const ol_attrs[] = { "type", "compact", "start", NULL} ;
static const char* const option_elt[] = { "option", NULL } ;
static const char* const optgroup_attrs[] = { ATTRS, "disabled", NULL } ;
static const char* const option_attrs[] = { ATTRS, "disabled", "label", "selected", "value", NULL } ;
static const char* const param_attrs[] = { "id", "value", "valuetype", "type", NULL } ;
static const char* const width_attr[] = { "width", NULL } ;
static const char* const pre_content[] = { PHRASE, "tt", "i", "b", "u", "s", "strike", "a", "br", "script", "map", "q", "span", "bdo", "iframe", NULL } ;
static const char* const script_attrs[] = { "charset", "src", "defer", "event", "for", NULL } ;
static const char* const language_attr[] = { "language", NULL } ;
static const char* const select_content[] = { "optgroup", "option", NULL } ;
static const char* const select_attrs[] = { ATTRS, "name", "size", "multiple", "disabled", "tabindex", "onfocus", "onblur", "onchange", NULL } ;
static const char* const style_attrs[] = { I18N, "media", "title", NULL } ;
static const char* const table_attrs[] = { ATTRS "summary", "width", "border", "frame", "rules", "cellspacing", "cellpadding", "datapagesize", NULL } ;
static const char* const table_depr[] = { "align", "bgcolor", NULL } ;
static const char* const table_contents[] = { "caption", "col", "colgroup", "thead", "tfoot", "tbody", "tr", NULL} ;
static const char* const tr_elt[] = { "tr", NULL } ;
static const char* const talign_attrs[] = { ATTRS, CELLHALIGN, CELLVALIGN, NULL} ;
static const char* const th_td_depr[] = { "nowrap", "bgcolor", "width", "height", NULL } ;
static const char* const th_td_attr[] = { ATTRS, "abbr", "axis", "headers", "scope", "rowspan", "colspan", CELLHALIGN, CELLVALIGN, NULL } ;
static const char* const textarea_attrs[] = { ATTRS, "name", "disabled", "readonly", "tabindex", "accesskey", "onfocus", "onblur", "onselect", "onchange", NULL } ;
static const char* const tr_contents[] = { "th", "td", NULL } ;
static const char* const bgcolor_attr[] = { "bgcolor", NULL } ;
static const char* const li_elt[] = { "li", NULL } ;
static const char* const ul_depr[] = { "type", "compact", NULL} ;
static const char* const dir_attr[] = { "dir", NULL} ;



#define DECL (const char**)

static const htmlElemDesc
html40ElementTable[] = {
{ "a",		0, 0, 0, 0, 0, 0, 1, "anchor ",
	DECL html_inline , NULL , DECL a_attrs , DECL target_attr, NULL
},
{ "abbr",	0, 0, 0, 0, 0, 0, 1, "abbreviated form",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "acronym",	0, 0, 0, 0, 0, 0, 1, "",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "address",	0, 0, 0, 0, 0, 0, 0, "information on author ",
	DECL inline_p  , NULL , DECL html_attrs, NULL, NULL
},
{ "applet",	0, 0, 0, 0, 1, 1, 2, "java applet ",
	DECL flow_param , NULL , NULL , DECL applet_attrs, NULL
},
{ "area",	0, 2, 2, 1, 0, 0, 0, "client-side image map area ",
	EMPTY ,  NULL , DECL area_attrs , DECL target_attr, DECL alt_attr
},
{ "b",		0, 3, 0, 0, 0, 0, 1, "bold text style",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "base",	0, 2, 2, 1, 0, 0, 0, "document base uri ",
	EMPTY , NULL , NULL , DECL target_attr, DECL href_attrs
},
{ "basefont",	0, 2, 2, 1, 1, 1, 1, "base font size " ,
	EMPTY , NULL , NULL, DECL basefont_attrs, NULL
},
{ "bdo",	0, 0, 0, 0, 0, 0, 1, "i18n bidi over-ride ",
	DECL html_inline , NULL , DECL core_i18n_attrs, NULL, DECL dir_attr
},
{ "big",	0, 3, 0, 0, 0, 0, 1, "large text style",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "blockquote",	0, 0, 0, 0, 0, 0, 0, "long quotation ",
	DECL html_flow , NULL , DECL quote_attrs , NULL, NULL
},
{ "body",	1, 1, 0, 0, 0, 0, 0, "document body ",
	DECL body_contents , "div" , DECL body_attrs, DECL body_depr, NULL
},
{ "br",		0, 2, 2, 1, 0, 0, 1, "forced line break ",
	EMPTY , NULL , DECL core_attrs, DECL clear_attrs , NULL
},
{ "button",	0, 0, 0, 0, 0, 0, 2, "push button ",
	DECL html_flow MODIFIER , NULL , DECL button_attrs, NULL, NULL
},
{ "caption",	0, 0, 0, 0, 0, 0, 0, "table caption ",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "center",	0, 3, 0, 0, 1, 1, 0, "shorthand for div align=center ",
	DECL html_flow , NULL , NULL, DECL html_attrs, NULL
},
{ "cite",	0, 0, 0, 0, 0, 0, 1, "citation",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "code",	0, 0, 0, 0, 0, 0, 1, "computer code fragment",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "col",	0, 2, 2, 1, 0, 0, 0, "table column ",
	EMPTY , NULL , DECL col_attrs , NULL, NULL
},
{ "colgroup",	0, 1, 0, 0, 0, 0, 0, "table column group ",
	DECL col_elt , "col" , DECL col_attrs , NULL, NULL
},
{ "dd",		0, 1, 0, 0, 0, 0, 0, "definition description ",
	DECL html_flow , NULL , DECL html_attrs, NULL, NULL
},
{ "del",	0, 0, 0, 0, 0, 0, 2, "deleted text ",
	DECL html_flow , NULL , DECL edit_attrs , NULL, NULL
},
{ "dfn",	0, 0, 0, 0, 0, 0, 1, "instance definition",
	DECL html_inline , NULL , DECL html_attrs, NULL, NULL
},
{ "dir",	0, 0, 0, 0, 1, 1, 0, "directory list",
	DECL blockli_elt, "li" , NULL, DECL compact_attrs, NULL
},
{ "div",	0, 0, 0, 0, 0, 0, 0, "generic language/style container",
	DECL html_flow, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "dl",		0, 0, 0, 0, 0, 0, 0, "definition list ",
	DECL dl_contents , "dd" , DECL html_attrs, DECL compact_attr, NULL
},
{ "dt",		0, 1, 0, 0, 0, 0, 0, "definition term ",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "em",		0, 3, 0, 0, 0, 0, 1, "emphasis",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "embed",	0, 1, 0, 0, 1, 1, 1, "generic embedded object ",
	EMPTY, NULL, DECL embed_attrs, NULL, NULL
},
{ "fieldset",	0, 0, 0, 0, 0, 0, 0, "form control group ",
	DECL fieldset_contents , NULL, DECL html_attrs, NULL, NULL
},
{ "font",	0, 3, 0, 0, 1, 1, 1, "local change to font ",
	DECL html_inline, NULL, NULL, DECL font_attrs, NULL
},
{ "form",	0, 0, 0, 0, 0, 0, 0, "interactive form ",
	DECL form_contents, "fieldset", DECL form_attrs , DECL target_attr, DECL action_attr
},
{ "frame",	0, 2, 2, 1, 0, 2, 0, "subwindow " ,
	EMPTY, NULL, NULL, DECL frame_attrs, NULL
},
{ "frameset",	0, 0, 0, 0, 0, 2, 0, "window subdivision" ,
	DECL frameset_contents, "noframes" , NULL , DECL frameset_attrs, NULL
},
{ "h1",		0, 0, 0, 0, 0, 0, 0, "heading ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "h2",		0, 0, 0, 0, 0, 0, 0, "heading ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "h3",		0, 0, 0, 0, 0, 0, 0, "heading ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "h4",		0, 0, 0, 0, 0, 0, 0, "heading ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "h5",		0, 0, 0, 0, 0, 0, 0, "heading ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "h6",		0, 0, 0, 0, 0, 0, 0, "heading ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "head",	1, 1, 0, 0, 0, 0, 0, "document head ",
	DECL head_contents, NULL, DECL head_attrs, NULL, NULL
},
{ "hr",		0, 2, 2, 1, 0, 0, 0, "horizontal rule " ,
	EMPTY, NULL, DECL html_attrs, DECL hr_depr, NULL
},
{ "html",	1, 1, 0, 0, 0, 0, 0, "document root element ",
	DECL html_content , NULL , DECL i18n_attrs, DECL version_attr, NULL
},
{ "i",		0, 3, 0, 0, 0, 0, 1, "italic text style",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "iframe",	0, 0, 0, 0, 0, 1, 2, "inline subwindow ",
	DECL html_flow, NULL, NULL, DECL iframe_attrs, NULL
},
{ "img",	0, 2, 2, 1, 0, 0, 1, "embedded image ",
	EMPTY, NULL, DECL img_attrs, DECL align_attr, DECL src_alt_attrs
},
{ "input",	0, 2, 2, 1, 0, 0, 1, "form control ",
	EMPTY, NULL, DECL input_attrs , DECL align_attr, NULL
},
{ "ins",	0, 0, 0, 0, 0, 0, 2, "inserted text",
	DECL html_flow, NULL, DECL edit_attrs, NULL, NULL
},
{ "isindex",	0, 2, 2, 1, 1, 1, 0, "single line prompt ",
	EMPTY, NULL, NULL, DECL prompt_attrs, NULL
},
{ "kbd",	0, 0, 0, 0, 0, 0, 1, "text to be entered by the user",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "label",	0, 0, 0, 0, 0, 0, 1, "form field label text ",
	DECL html_inline MODIFIER, NULL, DECL label_attrs , NULL, NULL
},
{ "legend",	0, 0, 0, 0, 0, 0, 0, "fieldset legend ",
	DECL html_inline, NULL, DECL legend_attrs , DECL align_attr, NULL
},
{ "li",		0, 1, 1, 0, 0, 0, 0, "list item ",
	DECL html_flow, NULL, DECL html_attrs, NULL, NULL
},
{ "link",	0, 2, 2, 1, 0, 0, 0, "a media-independent link ",
	EMPTY, NULL, DECL link_attrs, DECL target_attr, NULL
},
{ "map",	0, 0, 0, 0, 0, 0, 2, "client-side image map ",
	DECL map_contents , NULL, DECL html_attrs , NULL, DECL name_attr
},
{ "menu",	0, 0, 0, 0, 1, 1, 0, "menu list ",
	DECL blockli_elt , NULL, NULL, DECL compact_attrs, NULL
},
{ "meta",	0, 2, 2, 1, 0, 0, 0, "generic metainformation ",
	EMPTY, NULL, DECL meta_attrs , NULL , DECL content_attr
},
{ "noframes",	0, 0, 0, 0, 0, 2, 0, "alternate content container for non frame-based rendering ",
	DECL noframes_content, "body" , DECL html_attrs, NULL, NULL
},
{ "noscript",	0, 0, 0, 0, 0, 0, 0, "alternate content container for non script-based rendering ",
	DECL html_flow, "div", DECL html_attrs, NULL, NULL
},
{ "object",	0, 0, 0, 0, 0, 0, 2, "generic embedded object ",
	DECL object_contents , "div" , DECL object_attrs, DECL object_depr, NULL
},
{ "ol",		0, 0, 0, 0, 0, 0, 0, "ordered list ",
	DECL li_elt , "li" , DECL html_attrs, DECL ol_attrs, NULL
},
{ "optgroup",	0, 0, 0, 0, 0, 0, 0, "option group ",
	DECL option_elt , "option", DECL optgroup_attrs, NULL, DECL label_attr
},
{ "option",	0, 1, 0, 0, 0, 0, 0, "selectable choice " ,
	DECL html_pcdata, NULL, DECL option_attrs, NULL, NULL
},
{ "p",		0, 1, 0, 0, 0, 0, 0, "paragraph ",
	DECL html_inline, NULL, DECL html_attrs, DECL align_attr, NULL
},
{ "param",	0, 2, 2, 1, 0, 0, 0, "named property value ",
	EMPTY, NULL, DECL param_attrs, NULL, DECL name_attr
},
{ "pre",	0, 0, 0, 0, 0, 0, 0, "preformatted text ",
	DECL pre_content, NULL, DECL html_attrs, DECL width_attr, NULL
},
{ "q",		0, 0, 0, 0, 0, 0, 1, "short inline quotation ",
	DECL html_inline, NULL, DECL quote_attrs, NULL, NULL
},
{ "s",		0, 3, 0, 0, 1, 1, 1, "strike-through text style",
	DECL html_inline, NULL, NULL, DECL html_attrs, NULL
},
{ "samp",	0, 0, 0, 0, 0, 0, 1, "sample program output, scripts, etc.",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "script",	0, 0, 0, 0, 0, 0, 2, "script statements ",
	DECL html_cdata, NULL, DECL script_attrs, DECL language_attr, DECL type_attr
},
{ "select",	0, 0, 0, 0, 0, 0, 1, "option selector ",
	DECL select_content, NULL, DECL select_attrs, NULL, NULL
},
{ "small",	0, 3, 0, 0, 0, 0, 1, "small text style",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "span",	0, 0, 0, 0, 0, 0, 1, "generic language/style container ",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "strike",	0, 3, 0, 0, 1, 1, 1, "strike-through text",
	DECL html_inline, NULL, NULL, DECL html_attrs, NULL
},
{ "strong",	0, 3, 0, 0, 0, 0, 1, "strong emphasis",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "style",	0, 0, 0, 0, 0, 0, 0, "style info ",
	DECL html_cdata, NULL, DECL style_attrs, NULL, DECL type_attr
},
{ "sub",	0, 3, 0, 0, 0, 0, 1, "subscript",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "sup",	0, 3, 0, 0, 0, 0, 1, "superscript ",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "table",	0, 0, 0, 0, 0, 0, 0, "",
	DECL table_contents , "tr" , DECL table_attrs , DECL table_depr, NULL
},
{ "tbody",	1, 0, 0, 0, 0, 0, 0, "table body ",
	DECL tr_elt , "tr" , DECL talign_attrs, NULL, NULL
},
{ "td",		0, 0, 0, 0, 0, 0, 0, "table data cell",
	DECL html_flow, NULL, DECL th_td_attr, DECL th_td_depr, NULL
},
{ "textarea",	0, 0, 0, 0, 0, 0, 1, "multi-line text field ",
	DECL html_pcdata, NULL, DECL textarea_attrs, NULL, DECL rows_cols_attr
},
{ "tfoot",	0, 1, 0, 0, 0, 0, 0, "table footer ",
	DECL tr_elt , "tr" , DECL talign_attrs, NULL, NULL
},
{ "th",		0, 1, 0, 0, 0, 0, 0, "table header cell",
	DECL html_flow, NULL, DECL th_td_attr, DECL th_td_depr, NULL
},
{ "thead",	0, 1, 0, 0, 0, 0, 0, "table header ",
	DECL tr_elt , "tr" , DECL talign_attrs, NULL, NULL
},
{ "title",	0, 0, 0, 0, 0, 0, 0, "document title ",
	DECL html_pcdata, NULL, DECL i18n_attrs, NULL, NULL
},
{ "tr",		0, 0, 0, 0, 0, 0, 0, "table row ",
	DECL tr_contents , "td" , DECL talign_attrs, DECL bgcolor_attr, NULL
},
{ "tt",		0, 3, 0, 0, 0, 0, 1, "teletype or monospaced text style",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
},
{ "u",		0, 3, 0, 0, 1, 1, 1, "underlined text style",
	DECL html_inline, NULL, NULL, DECL html_attrs, NULL
},
{ "ul",		0, 0, 0, 0, 0, 0, 0, "unordered list ",
	DECL li_elt , "li" , DECL html_attrs, DECL ul_depr, NULL
},
{ "var",	0, 0, 0, 0, 0, 0, 1, "instance of a variable or program argument",
	DECL html_inline, NULL, DECL html_attrs, NULL, NULL
}
};


const htmlElemDesc *
htmlTagLookup(const xmlChar *tag) {
    unsigned int i;

    for (i = 0; i < (sizeof(html40ElementTable) /
                     sizeof(html40ElementTable[0]));i++) {
        if (!xmlStrcasecmp(tag, BAD_CAST html40ElementTable[i].name))
	    return((htmlElemDescPtr) &html40ElementTable[i]);
    }
    return(NULL);
}

static void
htmlAttrDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr doc, xmlAttrPtr cur,
	           const char *encoding ATTRIBUTE_UNUSED) {
    xmlChar *value;

    /*
     * TODO: The html output method should not escape a & character
     *       occurring in an attribute value immediately followed by
     *       a { character (see Section B.7.1 of the HTML 4.0 Recommendation).
     */

    if (cur == NULL) {
	return;
    }
    xmlOutputBufferWriteString(buf, " ");
    if ((cur->ns != NULL) && (cur->ns->prefix != NULL)) {
        xmlOutputBufferWriteString(buf, (const char *)cur->ns->prefix);
	xmlOutputBufferWriteString(buf, ":");
    }
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    if ((cur->children != NULL) && (!htmlIsBooleanAttr(cur->name))) {
	value = xmlNodeListGetString(doc, cur->children, 0);
	if (value) {
	    xmlOutputBufferWriteString(buf, "=");
	    if ((cur->ns == NULL) && (cur->parent != NULL) &&
		(cur->parent->ns == NULL) &&
		((!xmlStrcasecmp(cur->name, BAD_CAST "href")) ||
	         (!xmlStrcasecmp(cur->name, BAD_CAST "action")) ||
		 (!xmlStrcasecmp(cur->name, BAD_CAST "src")) ||
		 ((!xmlStrcasecmp(cur->name, BAD_CAST "name")) &&
		  (!xmlStrcasecmp(cur->parent->name, BAD_CAST "a"))))) {
		xmlChar *escaped;
		xmlChar *tmp = value;

		while (IS_BLANK_CH(*tmp)) tmp++;

		escaped = xmlURIEscapeStr(tmp, BAD_CAST"@/:=?;#%&,+");
		if (escaped != NULL) {
		    xmlBufferWriteQuotedString(buf->buffer, escaped);
		    xmlFree(escaped);
		} else {
		    xmlBufferWriteQuotedString(buf->buffer, value);
		}
	    } else {
		xmlBufferWriteQuotedString(buf->buffer, value);
	    }
	    xmlFree(value);
	} else  {
	    xmlOutputBufferWriteString(buf, "=\"\"");
	}
    }
}

static void
htmlSaveErr(int code, xmlNodePtr node, const char *extra)
{
    const char *msg = NULL;

    switch(code) {
        case XML_SAVE_NOT_UTF8:
	    msg = "string is not in UTF-8\n";
	    break;
	case XML_SAVE_CHAR_INVALID:
	    msg = "invalid character value\n";
	    break;
	case XML_SAVE_UNKNOWN_ENCODING:
	    msg = "unknown encoding %s\n";
	    break;
	case XML_SAVE_NO_DOCTYPE:
	    msg = "HTML has no DOCTYPE\n";
	    break;
	default:
	    msg = "unexpected error number\n";
    }
    __xmlSimpleError(XML_FROM_OUTPUT, code, node, msg, extra);
}


static void
htmlErrMemory(xmlParserCtxtPtr ctxt, const char *extra)
{
    if ((ctxt != NULL) && (ctxt->disableSAX != 0) &&
        (ctxt->instate == XML_PARSER_EOF))
	return;
    if (ctxt != NULL) {
        ctxt->errNo = XML_ERR_NO_MEMORY;
        ctxt->instate = XML_PARSER_EOF;
        ctxt->disableSAX = 1;
    }
    if (extra)
        __xmlRaiseError(NULL, NULL, NULL, ctxt, NULL, XML_FROM_PARSER,
                        XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, extra,
                        NULL, NULL, 0, 0,
                        "Memory allocation failed : %s\n", extra);
    else
        __xmlRaiseError(NULL, NULL, NULL, ctxt, NULL, XML_FROM_PARSER,
                        XML_ERR_NO_MEMORY, XML_ERR_FATAL, NULL, 0, NULL,
                        NULL, NULL, 0, 0, "Memory allocation failed\n");
}


/**
 * htmlIsBooleanAttr:
 * @name:  the name of the attribute to check
 *
 * Determine if a given attribute is a boolean attribute.
 * 
 * returns: false if the attribute is not boolean, true otherwise.
 */

static const char* htmlBooleanAttrs[] = {
  "checked", "compact", "declare", "defer", "disabled", "ismap",
  "multiple", "nohref", "noresize", "noshade", "nowrap", "readonly",
  "selected", NULL
};


int
htmlIsBooleanAttr(const xmlChar *name)
{
    int i = 0;

    while (htmlBooleanAttrs[i] != NULL) {
        if (xmlStrcasecmp((const xmlChar *)htmlBooleanAttrs[i], name) == 0)
            return 1;
        i++;
    }
    return 0;
}


int
htmlSetMetaEncoding(htmlDocPtr doc, const xmlChar *encoding) {
    htmlNodePtr cur, meta;
    const xmlChar *content;
    char newcontent[100];


    if (doc == NULL)
	return(-1);

    if (encoding != NULL) {
	snprintf(newcontent, sizeof(newcontent), "text/html; charset=%s",
                (char *)encoding);
	newcontent[sizeof(newcontent) - 1] = 0;
    }

    cur = doc->children;

    /*
     * Search the html
     */
    while (cur != NULL) {
	if ((cur->type == XML_ELEMENT_NODE) && (cur->name != NULL)) {
	    if (xmlStrcasecmp(cur->name, BAD_CAST"html") == 0)
		break;
	    if (xmlStrcasecmp(cur->name, BAD_CAST"head") == 0)
		goto found_head;
	    if (xmlStrcasecmp(cur->name, BAD_CAST"meta") == 0)
		goto found_meta;
	}
	cur = cur->next;
    }
    if (cur == NULL)
	return(-1);
    cur = cur->children;

    /*
     * Search the head
     */
    while (cur != NULL) {
	if ((cur->type == XML_ELEMENT_NODE) && (cur->name != NULL)) {
	    if (xmlStrcasecmp(cur->name, BAD_CAST"head") == 0)
		break;
	    if (xmlStrcasecmp(cur->name, BAD_CAST"meta") == 0)
		goto found_meta;
	}
	cur = cur->next;
    }
    if (cur == NULL)
	return(-1);
found_head:
    if (cur->children == NULL) {
	if (encoding == NULL)
	    return(0);
	meta = xmlNewDocNode(doc, NULL, BAD_CAST"meta", NULL);
	xmlAddChild(cur, meta);
	xmlNewProp(meta, BAD_CAST"http-equiv", BAD_CAST"Content-Type");
	xmlNewProp(meta, BAD_CAST"content", BAD_CAST newcontent);
	return(0);
    }
    cur = cur->children;

found_meta:
    if (encoding != NULL) {
	/*
	 * Create a new Meta element with the right attributes
	 */

	meta = xmlNewDocNode(doc, NULL, BAD_CAST"meta", NULL);
	xmlAddPrevSibling(cur, meta);
	xmlNewProp(meta, BAD_CAST"http-equiv", BAD_CAST"Content-Type");
	xmlNewProp(meta, BAD_CAST"content", BAD_CAST newcontent);
    }

    /*
     * Search and destroy all the remaining the meta elements carrying
     * encoding informations
     */
    while (cur != NULL) {
	if ((cur->type == XML_ELEMENT_NODE) && (cur->name != NULL)) {
	    if (xmlStrcasecmp(cur->name, BAD_CAST"meta") == 0) {
		xmlAttrPtr attr = cur->properties;
		int http;
		const xmlChar *value;

		content = NULL;
		http = 0;
		while (attr != NULL) {
		    if ((attr->children != NULL) &&
		        (attr->children->type == XML_TEXT_NODE) &&
		        (attr->children->next == NULL)) {
			value = attr->children->content;
			if ((!xmlStrcasecmp(attr->name, BAD_CAST"http-equiv"))
			 && (!xmlStrcasecmp(value, BAD_CAST"Content-Type")))
			    http = 1;
			else 
                        {
                           if ((value != NULL) && 
				(!xmlStrcasecmp(attr->name, BAD_CAST"content")))
			      content = value;
                        }
		        if ((http != 0) && (content != NULL))
			    break;
		    }
		    attr = attr->next;
		}
		if ((http != 0) && (content != NULL)) {
		    meta = cur;
		    cur = cur->next;
		    xmlUnlinkNode(meta);
                    xmlFreeNode(meta);
		    continue;
		}

	    }
	}
	cur = cur->next;
    }
    return(0);
}


htmlDocPtr
htmlNewDocNoDtD(const xmlChar *URI, const xmlChar *ExternalID) {
    xmlDocPtr cur;

    /*
     * Allocate a new document and fill the fields.
     */
    cur = (xmlDocPtr) xmlMalloc(sizeof(xmlDoc));
    if (cur == NULL) {
	htmlErrMemory(NULL, "HTML document creation failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xmlDoc));

    cur->type = XML_HTML_DOCUMENT_NODE;
    cur->version = NULL;
    cur->intSubset = NULL;
    cur->doc = cur;
    cur->name = NULL;
    cur->children = NULL; 
    cur->extSubset = NULL;
    cur->oldNs = NULL;
    cur->encoding = NULL;
    cur->standalone = 1;
    cur->compression = 0;
    cur->ids = NULL;
    cur->refs = NULL;
    cur->_private = NULL;
    cur->charset = XML_CHAR_ENCODING_UTF8;
    if ((ExternalID != NULL) ||
	(URI != NULL))
	xmlCreateIntSubset(cur, BAD_CAST "html", ExternalID, URI);
    return(cur);
}

/**
 * htmlNewDoc:
 * @URI:  URI for the dtd, or NULL
 * @ExternalID:  the external ID of the DTD, or NULL
 *
 * Creates a new HTML document
 *
 * Returns a new document
 */
htmlDocPtr
htmlNewDoc(const xmlChar *URI, const xmlChar *ExternalID) {
    if ((URI == NULL) && (ExternalID == NULL))
	return(htmlNewDocNoDtD(
		    BAD_CAST "http://www.w3.org/TR/REC-html40/loose.dtd",
		    BAD_CAST "-//W3C//DTD HTML 4.0 Transitional//EN"));

    return(htmlNewDocNoDtD(URI, ExternalID));
}



static void
htmlAttrListDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr doc, xmlAttrPtr cur, const char *encoding) {
    if (cur == NULL) {
	return;
    }
    while (cur != NULL) {
        htmlAttrDumpOutput(buf, doc, cur, encoding);
	cur = cur->next;
    }
}


void
htmlNodeDumpFormatOutput(xmlOutputBufferPtr buf, xmlDocPtr doc,
	                 xmlNodePtr cur, const char *encoding, int format) {
    const htmlElemDesc * info;

    xmlInitParser();

    if ((cur == NULL) || (buf == NULL)) {
	return;
    }
    /*
     * Special cases.
     */
    if (cur->type == XML_DTD_NODE)
	return;
    if ((cur->type == XML_HTML_DOCUMENT_NODE) ||
        (cur->type == XML_DOCUMENT_NODE)){
	htmlDocContentDumpOutput(buf, (xmlDocPtr) cur, encoding);
	return;
    }
    if (cur->type == XML_ATTRIBUTE_NODE) {
        htmlAttrDumpOutput(buf, doc, (xmlAttrPtr) cur, encoding);
	return;
    }
    if (cur->type == HTML_TEXT_NODE) {
	if (cur->content != NULL) {
	    if (((cur->name == (const xmlChar *)xmlStringText) ||
		 (cur->name != (const xmlChar *)xmlStringTextNoenc)) &&
		((cur->parent == NULL) ||
		 ((xmlStrcasecmp(cur->parent->name, BAD_CAST "script")) &&
		  (xmlStrcasecmp(cur->parent->name, BAD_CAST "style"))))) {
		xmlChar *buffer;

		buffer = xmlEncodeEntitiesReentrant(doc, cur->content);
		if (buffer != NULL) {
		    xmlOutputBufferWriteString(buf, (const char *)buffer);
		    xmlFree(buffer);
		}
	    } else {
		xmlOutputBufferWriteString(buf, (const char *)cur->content);
	    }
	}
	return;
    }
    if (cur->type == HTML_COMMENT_NODE) {
	if (cur->content != NULL) {
	    xmlOutputBufferWriteString(buf, "<!--");
	    xmlOutputBufferWriteString(buf, (const char *)cur->content);
	    xmlOutputBufferWriteString(buf, "-->");
	}
	return;
    }
    if (cur->type == HTML_PI_NODE) {
	if (cur->name == NULL)
	    return;
	xmlOutputBufferWriteString(buf, "<?");
	xmlOutputBufferWriteString(buf, (const char *)cur->name);
	if (cur->content != NULL) {
	    xmlOutputBufferWriteString(buf, " ");
	    xmlOutputBufferWriteString(buf, (const char *)cur->content);
	}
	xmlOutputBufferWriteString(buf, ">");
	return;
    }
    if (cur->type == HTML_ENTITY_REF_NODE) {
        xmlOutputBufferWriteString(buf, "&");
	xmlOutputBufferWriteString(buf, (const char *)cur->name);
        xmlOutputBufferWriteString(buf, ";");
	return;
    }
    if (cur->type == HTML_PRESERVE_NODE) {
	if (cur->content != NULL) {
	    xmlOutputBufferWriteString(buf, (const char *)cur->content);
	}
	return;
    }

    /*
     * Get specific HTML info for that node.
     */
    if (cur->ns == NULL)
	info = htmlTagLookup(cur->name);
    else
	info = NULL;

    xmlOutputBufferWriteString(buf, "<");
    if ((cur->ns != NULL) && (cur->ns->prefix != NULL)) {
        xmlOutputBufferWriteString(buf, (const char *)cur->ns->prefix);
	xmlOutputBufferWriteString(buf, ":");
    }
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    if (cur->nsDef)
	xmlNsListDumpOutput(buf, cur->nsDef);
    if (cur->properties != NULL)
        htmlAttrListDumpOutput(buf, doc, cur->properties, encoding);

    if ((info != NULL) && (info->empty)) {
        xmlOutputBufferWriteString(buf, ">");
	if ((format) && (!info->isinline) && (cur->next != NULL)) {
	    if ((cur->next->type != HTML_TEXT_NODE) &&
		(cur->next->type != HTML_ENTITY_REF_NODE) &&
		(cur->parent != NULL) &&
		(cur->parent->name != NULL) &&
		(cur->parent->name[0] != 'p')) /* p, pre, param */
		xmlOutputBufferWriteString(buf, "\n");
	}
	return;
    }
    if (((cur->type == XML_ELEMENT_NODE) || (cur->content == NULL)) &&
	(cur->children == NULL)) {
        if ((info != NULL) && (info->saveEndTag != 0) &&
	    (xmlStrcmp(BAD_CAST info->name, BAD_CAST "html")) &&
	    (xmlStrcmp(BAD_CAST info->name, BAD_CAST "body"))) {
	    xmlOutputBufferWriteString(buf, ">");
	} else {
	    xmlOutputBufferWriteString(buf, "></");
            if ((cur->ns != NULL) && (cur->ns->prefix != NULL)) {
                xmlOutputBufferWriteString(buf, (const char *)cur->ns->prefix);
                xmlOutputBufferWriteString(buf, ":");
            }
	    xmlOutputBufferWriteString(buf, (const char *)cur->name);
	    xmlOutputBufferWriteString(buf, ">");
	}
	if ((format) && (cur->next != NULL) &&
            (info != NULL) && (!info->isinline)) {
	    if ((cur->next->type != HTML_TEXT_NODE) &&
		(cur->next->type != HTML_ENTITY_REF_NODE) &&
		(cur->parent != NULL) &&
		(cur->parent->name != NULL) &&
		(cur->parent->name[0] != 'p')) /* p, pre, param */
		xmlOutputBufferWriteString(buf, "\n");
	}
	return;
    }
    xmlOutputBufferWriteString(buf, ">");
    if ((cur->type != XML_ELEMENT_NODE) &&
	(cur->content != NULL)) {
	    /*
	     * Uses the OutputBuffer property to automatically convert
	     * invalids to charrefs
	     */

            xmlOutputBufferWriteString(buf, (const char *) cur->content);
    }
    if (cur->children != NULL) {
        if ((format) && (info != NULL) && (!info->isinline) &&
	    (cur->children->type != HTML_TEXT_NODE) &&
	    (cur->children->type != HTML_ENTITY_REF_NODE) &&
	    (cur->children != cur->last) &&
	    (cur->name != NULL) &&
	    (cur->name[0] != 'p')) /* p, pre, param */
	    xmlOutputBufferWriteString(buf, "\n");
	htmlNodeListDumpOutput(buf, doc, cur->children, encoding, format);
        if ((format) && (info != NULL) && (!info->isinline) &&
	    (cur->last->type != HTML_TEXT_NODE) &&
	    (cur->last->type != HTML_ENTITY_REF_NODE) &&
	    (cur->children != cur->last) &&
	    (cur->name != NULL) &&
	    (cur->name[0] != 'p')) /* p, pre, param */
	    xmlOutputBufferWriteString(buf, "\n");
    }
    xmlOutputBufferWriteString(buf, "</");
    if ((cur->ns != NULL) && (cur->ns->prefix != NULL)) {
        xmlOutputBufferWriteString(buf, (const char *)cur->ns->prefix);
	xmlOutputBufferWriteString(buf, ":");
    }
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    xmlOutputBufferWriteString(buf, ">");
    if ((format) && (info != NULL) && (!info->isinline) &&
	(cur->next != NULL)) {
        if ((cur->next->type != HTML_TEXT_NODE) &&
	    (cur->next->type != HTML_ENTITY_REF_NODE) &&
	    (cur->parent != NULL) &&
	    (cur->parent->name != NULL) &&
	    (cur->parent->name[0] != 'p')) /* p, pre, param */
	    xmlOutputBufferWriteString(buf, "\n");
    }
}


//static void
int htmlNodeListDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr doc,
	               xmlNodePtr cur, const char *encoding, int format) {
    if (cur == NULL) {
	return 0;
    }
    while (cur != NULL) {
        htmlNodeDumpFormatOutput(buf, doc, cur, encoding, format);
	cur = cur->next;
    }
    return 0;
}



static void
htmlDtdDumpOutput(xmlOutputBufferPtr buf, xmlDocPtr doc,
	          const char *encoding ATTRIBUTE_UNUSED) {
    xmlDtdPtr cur = doc->intSubset;

    if (cur == NULL) {
	htmlSaveErr(XML_SAVE_NO_DOCTYPE, (xmlNodePtr) doc, NULL);
	return;
    }
    xmlOutputBufferWriteString(buf, "<!DOCTYPE ");
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    if (cur->ExternalID != NULL) {
	xmlOutputBufferWriteString(buf, " PUBLIC ");
	xmlBufferWriteQuotedString(buf->buffer, cur->ExternalID);
	if (cur->SystemID != NULL) {
	    xmlOutputBufferWriteString(buf, " ");
	    xmlBufferWriteQuotedString(buf->buffer, cur->SystemID);
	} 
    }  else if (cur->SystemID != NULL) {
	xmlOutputBufferWriteString(buf, " SYSTEM ");
	xmlBufferWriteQuotedString(buf->buffer, cur->SystemID);
    }
    xmlOutputBufferWriteString(buf, ">\n");
}


void
htmlDocContentDumpFormatOutput(xmlOutputBufferPtr buf, xmlDocPtr cur,
	                       const char *encoding, int format) {
    int type;

    xmlInitParser();

    if ((buf == NULL) || (cur == NULL))
        return;

    /*
     * force to output the stuff as HTML, especially for entities
     */
    type = cur->type;
    cur->type = XML_HTML_DOCUMENT_NODE;
    if (cur->intSubset != NULL) {
        htmlDtdDumpOutput(buf, cur, NULL);
    }
    if (cur->children != NULL) {
        htmlNodeListDumpOutput(buf, cur, cur->children, encoding, format);
    }
    xmlOutputBufferWriteString(buf, "\n");
    cur->type = (xmlElementType) type;
}

