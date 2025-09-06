export async function GET() {
  const eventName = process.env["EVENT_NAME"] ?? "";
  const eventPayload = JSON.parse(process.env["EVENT_PAYLOAD"] ?? "{}");

  if (eventName === "release" && eventPayload.action === "published") {
    const tagName = eventPayload.release.tag_name;
    return new Response(tagName, {
      status: 200,
      headers: {
        "Content-Type": "text/plain",
      },
    });
  }

  // Fallback: Fetch the latest release from GitHub API
  const response = await fetch(
    "https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest",
  );
  const data = await response.json();
  const tagName = data.tag_name;
  return new Response(tagName, {
    status: 200,
    headers: {
      "Content-Type": "text/plain",
    },
  });
}
