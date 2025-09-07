import { getLatestRelease } from "../../lib/github";

export async function GET() {
  const release = await getLatestRelease();
  return new Response(JSON.stringify(release, null, 2), {
    status: 200,
    headers: {
      "Content-Type": "application/json",
    },
  });
}
