export async function GET() {
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
