#!/usr/bin/env python3
# ─── tools/routes.py ─────────────────────────────────────────────
#
# THE ROUTE LIST HAS ONE HOME: web/routes.json. Four pages carry the same
# sandwich menu — the engine shell, home/, gallery/, writings/ — and
# this is the one renderer all the dist scripts call, so a route added to
# the JSON appears on every page with no markup edit anywhere. The menu's
# rules have one home too, web/menu.css, inlined by menu_css(). Neither
# file ships; both are build-time only.
#
#   side    "site" | "engine" | absent (both)
#   href    an ABSOLUTE site path ("/home/#text"). The renderer speaks it
#           RELATIVE to the page it is rendering (rel, below): the site's
#           own convention (../fonts/, ../shared.css) and the collection
#           gate's jurisdiction line — a "../" reference is the site's, a
#           bare one is the collection's, and a leading slash is neither.
#           A route that names the page it is on is dropped: a menu does
#           not list where you are. (DOORS_1, after the gate refused.)
#   return  true — on a site page, when this tab was opened by the engine
#           (window.opener, same origin), the link CLOSES the tab: the world
#           is still there, where it was. Without an opener it navigates.
#           The engine's own links open a new tab with rel="opener" for
#           exactly this, whatever the visitor browses in between.
#   engine  "pane" — on the engine shell the route opens a pane inside
#           the menu (the shell owns the pane's content); its href, if
#           any, is the pane's door out. A site page renders it as a
#           link if it has an href and skips it otherwise.
#
#   follow  web/follow.json — the five external links, rendered by
#           follow_html(). The ENGINE'S FOLLOW PANE is its only reader
#           since DOORS_4 ruling 3 — the site footers that carried it are
#           gone. One list, one home, one place it is shown.
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(os.path.dirname(HERE), "web")
ROUTES = os.path.join(WEB, "routes.json")
MENU_CSS = os.path.join(WEB, "menu.css")
FOLLOW = os.path.join(WEB, "follow.json")


def esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;")
            .replace(">", "&gt;").replace('"', "&quot;"))


def load():
    with open(ROUTES, encoding="utf-8") as fh:
        routes = json.load(fh)
    for r in routes:
        if "id" not in r or "label" not in r:
            raise SystemExit("REFUSE  web/routes.json: every route needs id and label")
        if r.get("side") not in (None, "site", "engine"):
            raise SystemExit("REFUSE  web/routes.json: side must be site, engine or absent (%s)" % r["id"])
    return routes


def rel(href, base):
    """An absolute site path, spoken from a page at `base` (a directory,
    "/gallery/"). "/home/#text" is "../home/#text" from /gallery/
    and "#text" from /home/; a page's own address is "./"."""
    path, frag = (href.split("#", 1) + [""])[:2]
    frag = ("#" + frag) if frag else ""
    b = [seg for seg in base.split("/") if seg]
    t = [seg for seg in path.split("/") if seg]
    i = 0
    while i < len(b) and i < len(t) and b[i] == t[i]:
        i += 1
    parts = [".."] * (len(b) - i) + t[i:]
    if not parts:
        return frag or "./"
    out = "/".join(parts)
    if path.endswith("/"):
        out += "/"
    return out + frag


def nav_html(side, base, indent="    "):
    """The <nav> body for one shell, spoken from the page at `base`."""
    items = []
    for r in load():
        if r.get("side") not in (None, side):
            continue
        href = rel(r["href"], base) if r.get("href") else ""
        if href == "./":
            continue   # the page itself
        if side == "engine" and r.get("engine") == "pane":
            data = (' data-href="%s"' % esc(href)) if href else ""
            items.append('<button type="button" data-route="%s" data-pane="%s"%s>%s</button>'
                         % (esc(r["id"]), esc(r["id"]), data, esc(r["label"])))
            continue
        if not href:
            continue
        attrs = ""
        if side == "engine":
            # DOORS_2 — the world tab is never left; rel=opener because
            # _blank implies noopener since 2021 and the return path needs it.
            attrs = ' target="_blank" rel="opener"'
        elif r.get("return"):
            # DOORS_2 — close-or-navigate: close() only counts if the tab
            # actually closed (window.closed), else the link runs.
            attrs = ' onclick="return !(window.opener &amp;&amp; !window.opener.closed &amp;&amp; (window.close(), window.closed))"'
        items.append('<a data-route="%s" href="%s"%s>%s</a>'
                     % (esc(r["id"]), esc(href), attrs, esc(r["label"])))
    # DOORS_4 — WRITE ME IS A BOX, NOT A PAGE BAND, and it lives in the
    # site's sandwich under the links. The engine keeps its own pane.
    if side == "site":
        items.append(follow_box_html(indent=indent))       # Jean's ask, after DOORS_4
        items.append(write_box_html(base, indent=indent))
    return ("\n" + indent).join(items)


def menu_css():
    with open(MENU_CSS, encoding="utf-8") as fh:
        return fh.read().rstrip("\n")


def follow_box_html(indent="  "):
    """Follow for more, as a small box inside the site sandwich — Jean's
    ask, and the same shape the write box already uses so the sandwich
    holds two boxes and not two idioms.

    DOORS_4 ruling 3 put the social links in ONE place, the engine's
    Follow pane, and took the nav off the three site footers. This does
    NOT undo that: the footers stay gone (ruling 8 keeps the name and the
    colophon where they are) and the links come back INSIDE THE MENU,
    which is where Jean asked for them. web/follow.json is still their
    one home; follow_html() is still the one renderer."""
    lines = ['<details class="followbox">', '  <summary>Follow for more</summary>']
    for line in follow_html(indent="  ").split("\n"):
        lines.append("  " + line.strip())
    lines.append('</details>')
    return ("\n" + indent).join(lines)


def write_box_html(base, indent="  "):
    """Write me, as a small box inside the site sandwich (DOORS_4): a
    nested <details>, the same fields the engine's pane sends, posting to
    the same function. The inline script is the courtesy layer — without
    it the native POST still lands, and the function answers in JSON.

    NOTE, and it is a real one: the about page's old band showed
    site.json's address when the endpoint refused. This box cannot — it
    renders for every site page from a file that has never read
    site.json, and only about_dist's fill() knows __EMAIL__. The engine's
    Write pane keeps the mailto fallback; the site's box says try again.
    Registered for Jean."""
    action = rel("/api/message", base)
    lines = [
        '<details class="writebox">',
        '  <summary>Write me</summary>',
        '  <form method="post" action="%s">' % esc(action),
        '    <label>name<input name="name" autocomplete="name"></label>',
        '    <label>email, if you want an answer<input name="email" type="email" autocomplete="email"></label>',
        '    <label>message<textarea name="message" required rows="4"></textarea></label>',
        '    <label class="hp" aria-hidden="true">leave this empty<input name="website" tabindex="-1" autocomplete="off"></label>',
        '    <div class="row"><button type="submit">Send</button><span class="said" role="status"></span></div>',
        '  </form>',
        '  <script>(function () {',
        '    var d = document.currentScript.closest("details");',
        '    var f = d.querySelector("form"), s = d.querySelector(".said");',
        '    var b = f.querySelector("button[type=submit]");',
        '    f.addEventListener("submit", function (e) {',
        '      e.preventDefault();',
        '      if (b.disabled) return;              // the band this box replaced guarded the double click; so does it',
        '      b.disabled = true; s.textContent = "Sending…";',
        '      var data = {}; ["name", "email", "message", "website"].forEach(function (k) {',
        '        var el = f.elements[k]; data[k] = el ? el.value : "";',
        '      });',
        '      fetch(f.action, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(data) })',
        '        .then(function (r) { if (!r.ok) throw new Error("HTTP " + r.status); s.textContent = "Sent. Thank you."; f.reset(); })',
        '        .catch(function () { s.textContent = "It did not go through — try again in a moment."; })',
        '        .then(function () { b.disabled = false; });',
        '    });',
        '  })();</script>',
        '</details>',
    ]
    return ("\n" + indent).join(lines)


def follow_html(indent="    "):
    """Follow for more — the five external links, one home (web/follow.json).
    External links, so the collection gate exempts them (https:).
    It WAS the website's bottom menu (DOORS_3); since DOORS_4 ruling 3 it
    has ONE reader, the engine's Follow pane. web_dist injects it;
    about_dist and collection_dist no longer do."""
    with open(FOLLOW, encoding="utf-8") as fh:
        links = json.load(fh)
    items = []
    for l in links:
        if not l.get("label") or not str(l.get("href", "")).startswith("https://"):
            raise SystemExit("REFUSE  web/follow.json: every entry needs a label and an https href")
        items.append('<a href="%s" rel="me noopener" target="_blank">%s</a>' % (esc(l["href"]), esc(l["label"])))
    return ("\n" + indent).join(items)


if __name__ == "__main__":
    for base in ("/home/", "/gallery/"):
        print("--- site @ %s" % base)
        print(nav_html("site", base))
    print("--- engine @ /")
    print(nav_html("engine", "/"))
    print("--- follow")
    print(follow_html())
