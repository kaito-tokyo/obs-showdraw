export async function GET() {
  const eventName = process.env["EVENT_NAME"] ?? "";
  const eventPayload = JSON.parse(process.env["EVENT_PAYLOAD"] ?? "{}");

  if (eventName === "release" && eventPayload.action === "published") {
    return new Response(JSON.stringify(eventPayload.release, null, 2), {
      status: 200,
      headers: {
        "Content-Type": "application/json",
      },
    });
  }

  // Fallback: Fetch the latest release from GitHub API
  const response = await fetch(
    "https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest",
  );
  const data = await response.json();
  return new Response(JSON.stringify(data, null, 2), {
    status: 200,
    headers: {
      "Content-Type": "application/json",
    },
  });
}
