#!/usr/bin/env python3
# ─── tools/routes.py ─────────────────────────────────────────────
#
# THE ROUTE LIST HAS ONE HOME: web/routes.json. Three pages carry the
# same sandwich menu — the engine shell, about/, collection/ — and this
# is the one renderer all three dist scripts call, so a route added to
# the JSON appears on every page with no markup edit anywhere. The
# menu's rules have one home too, web/menu.css, inlined by menu_css().
# Neither file ships; both are build-time only.
#
#   side    "site" | "engine" | absent (both)
#   return  true — on a site page, when this tab was opened by the engine
#           (window.opener, same origin), the link CLOSES the tab: the world
#           is still there, where it was. Without an opener it navigates.
#           The engine's own links open a new tab with rel="opener" for
#           exactly this, whatever the visitor browses in between.
#   engine  "pane" — on the engine shell the route opens a pane inside
#           the menu (the shell owns the pane's content); its href, if
#           any, is the pane's door out. A site page renders it as a
#           link if it has an href and skips it otherwise.
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(os.path.dirname(HERE), "web")
ROUTES = os.path.join(WEB, "routes.json")
MENU_CSS = os.path.join(WEB, "menu.css")


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


def nav_html(side, indent="    "):
    """The <nav> body for one shell. Absolute hrefs, so the same markup is
    right at /, /about/ and /collection/."""
    items = []
    for r in load():
        if r.get("side") not in (None, side):
            continue
        if side == "engine" and r.get("engine") == "pane":
            href = (' data-href="%s"' % esc(r["href"])) if r.get("href") else ""
            items.append('<button type="button" data-route="%s" data-pane="%s"%s>%s</button>'
                         % (esc(r["id"]), esc(r["id"]), href, esc(r["label"])))
            continue
        if not r.get("href"):
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
                     % (esc(r["id"]), esc(r["href"]), attrs, esc(r["label"])))
    return ("\n" + indent).join(items)


def menu_css():
    with open(MENU_CSS, encoding="utf-8") as fh:
        return fh.read().rstrip("\n")


if __name__ == "__main__":
    print(nav_html("site"))
    print("---")
    print(nav_html("engine"))
