export async function GET() {
  const response = await fetch("https://api.github.com/repos/kaito-tokyo/obs-showdraw/releases/latest");
  const data = await response.json();
  return new Response(JSON.stringify(data, null, 2), {
    status: 200,
    headers: {
      "Content-Type": "application/json",
    },
  });
}
