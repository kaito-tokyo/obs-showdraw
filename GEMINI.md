# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- C and C++ files must be formatted using clang-format-19 after any modification.
- CMake files must be formatted using gersemi after any modification.
- OBS team maintains the CMake and GitHub Actions so we don't need to improve these parts.

## How to build and run tests on macOS

1. Run `cmake --preset macos -DBUILD_TESTING=ON` if CMake-related files were changed.
2. Run `cmake --build --preset macos`.
3. Run `ctest --preset macos --rerun-failed --output-on-failure`.

## Release Automation with Gemini

To initiate a new release, follow these steps, and Gemini can assist you:

1.  **Update Version in `buildspec.json`**:
    *   You will be prompted to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    *   Gemini will then update the `version` field in `buildspec.json` with this new version.
2.  **Create a Pull Request (PR)**:
    *   After updating `buildspec.json`, you should create a new branch, commit the change, and open a Pull Request for review.
    *   **Important**: The release process will pause here, awaiting the merge of this PR.
3.  **Push Git Tag (After PR Merge)**:
    *   Once your PR (containing the `buildspec.json` version update) is merged into the main branch (e.g., `master` or `main`), you can then instruct Gemini to push the corresponding Git tag.
    *   **Tag Format Constraint**: The tag must be in the format `X.Y.Z` (e.g., `1.2.3`), without a 'v' prefix.
    *   Pushing the tag will trigger the automated release workflow in GitHub Actions.
4.  **Monitor Release Progress**:
    *   Gemini will provide updates on the release workflow's progress after the tag is pushed.

**Example Interaction:**

User: `リリースを開始して` (Start the release)
Gemini: `はい、新しいリリースを開始します。まず、buildspec.json に記載するリリースバージョンを指定してください (例: 1.0.0, 1.0.0-beta1)。` (Okay, I will start a new release. First, please specify the release version to be written in buildspec.json (e.g., 1.0.0, 1.0.0-beta1).)
User: `1.2.3`
Gemini: `buildspec.json のバージョンを 1.2.3 に更新します。よろしいですか？` (I will update the version in buildspec.json to 1.2.3. Is that okay?)
User: `はい` (Yes)
Gemini: `buildspec.json を更新しました。この変更を含むPRを作成し、マージしてください。PRがマージされたら、私にタグをプッシュするように指示してください。` (Updated buildspec.json. Please create a PR with this change and merge it. Once the PR is merged, instruct me to push the tag.)
User: `タグをプッシュして` (Push the tag)
Gemini: `バージョン 1.2.3 の Git タグをプッシュします。よろしいですか？` (I will push Git tag for version 1.2.3. Is that okay?)
User: `はい` (Yes)
Gemini: `Git タグ '1.2.3' をプッシュしました。GitHub Actions でリリースワークフローが開始されます。進捗を監視します。` (Pushed Git tag '1.2.3'. The release workflow will start in GitHub Actions. I will monitor the progress.)
Gemini: `GitHub リリースドラフトのURL: https://github.com/kaito-tokyo/obs-showdraw/releases/new?tag=1.2.3` (GitHub Release Draft URL: https://github.com/kaito-tokyo/obs-showdraw/releases/new?tag=1.2.3)
