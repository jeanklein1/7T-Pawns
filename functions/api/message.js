// ─── functions/api/message.js ────────────────────────────────────
//
// The message box's other half. Cloudflare Pages picks this up from
// the repo-root functions/ directory and serves it at /api/message —
// the only dynamic surface on the whole site, and it stays this small.
//
// Configuration (Pages → Settings → Environment variables, Production;
// a change takes effect on the NEXT deploy):
//   RESEND_API_KEY   an api key from resend.com (free tier is plenty)
//   MESSAGE_TO       the inbox that receives the box's messages
//   MESSAGE_FROM     optional — the sender, "the board <board@everexpandingboard.com>"
//                    once the domain is verified in Resend. Absent, the
//                    default below: Resend's unverified sender, which
//                    delivers ONLY to the Resend account's own address, so
//                    MESSAGE_TO must be that address until the domain is.
//
// Unconfigured, it answers 501 — the box never silently eats a message
// (HOUSE_0 read exactly that from the live site: 501 "unconfigured", the
// two variables unset). The sender service is deliberately swappable:
// everything provider-specific is inside sendViaResend, and nothing
// else knows it exists.

const MAX_LEN = 8000;

export async function onRequestPost({ request, env }) {
  let body;
  try {
    body = await parseBody(request);
  } catch {
    return answer(400, { error: "unreadable" });
  }

  // the honeypot: humans never see the field, machines love to fill it.
  // Lie politely and drop it.
  if (body.website) return answer(200, { ok: true });

  const message = (body.message || "").trim();
  if (!message) return answer(400, { error: "empty" });

  const name = (body.name || "").trim().slice(0, 200);
  const email = (body.email || "").trim().slice(0, 200);

  if (!env.RESEND_API_KEY || !env.MESSAGE_TO) {
    return answer(501, { error: "unconfigured" });
  }

  const ok = await sendViaResend(env, {
    subject: "the board: a message" + (name ? " from " + name : ""),
    text:
      message.slice(0, MAX_LEN) +
      "\n\n—\n" +
      (name ? "name:  " + name + "\n" : "") +
      (email ? "reply: " + email + "\n" : "(no reply address left)\n"),
    replyTo: email || undefined,
  });

  return ok ? answer(200, { ok: true }) : answer(502, { error: "send failed" });
}

async function parseBody(request) {
  const type = request.headers.get("content-type") || "";
  if (type.includes("application/json")) return await request.json();
  // no-JavaScript visitors arrive as a plain form post
  const form = await request.formData();
  return Object.fromEntries(form.entries());
}

async function sendViaResend(env, { subject, text, replyTo }) {
  const r = await fetch("https://api.resend.com/emails", {
    method: "POST",
    headers: {
      Authorization: "Bearer " + env.RESEND_API_KEY,
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      from: env.MESSAGE_FROM || "the board <jean@everexpandingboard.com>",
      to: env.MESSAGE_TO,
      subject,
      text,
      reply_to: replyTo,
    }),
  });
  return r.ok;
}

function answer(status, obj) {
  return new Response(JSON.stringify(obj), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}
