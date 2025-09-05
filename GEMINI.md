# Development Guideline

- This project must be developed in C++17.
- Implementation related to unique_ptr should be placed under bridge.hpp.
- C and C++ files must be formatted using clang-format-19 after any modification.
- CMake files must be formatted using gersemi after any modification.
- OBS team maintains the CMake and GitHub Actions so we don't need to improve these parts.
- The default branch of this project is `main`.

## How to build and run tests on macOS

1. Run `cmake --preset macos -DBUILD_TESTING=ON` if CMake-related files were changed.
2. Run `cmake --build --preset macos`.
3. Run `ctest --preset macos --rerun-failed --output-on-failure`.

## Release Automation with Gemini

To initiate a new release, instruct Gemini to start the process (e.g., "リリースを開始して"). Gemini will then guide you through the following steps:

1.  **Specify New Version**:
    *   Gemini will display the current version.
    *   You will be prompted to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    *   **Constraint**: The version must follow Semantic Versioning (e.g., `MAJOR.MINOR.PATCH`).

2.  **Prepare & Update `buildspec.json`**:
    *   Gemini will confirm the current branch is `main` and synchronized with the remote.
    *   A new branch (`bump-X.Y.Z`) will be created.
    *   The `version` field in `buildspec.json` will be updated.

3.  **Create & Merge Pull Request (PR)**:
    *   Gemini will create a PR for the version update.
    *   You will be instructed to merge this PR.
    *   **Pause**: The process pauses until you confirm the PR merge to Gemini.

4.  **Push Git Tag**:
    *   After PR merge confirmation, instruct Gemini to push the Git tag.
    *   **Constraint**: The tag must be `X.Y.Z` (no 'v' prefix).
    *   Pushing the tag triggers the automated release workflow.

5.  **Finalize Release**:
    *   Gemini will provide the draft release URL.
    *   You should then complete the release on GitHub.

**Example Interaction:**

User: `リリースを開始して`
Gemini: `はい、新しいリリースを開始します。現在のバージョンは X.Y.Z です。buildspec.json に記載するリリースバージョンを指定してください (例: 1.0.0, 1.0.0-beta1)。`
User: `1.2.3`
Gemini: `buildspec.json のバージョンを 1.2.3 に更新します。よろしいですか？`
User: `はい`
Gemini: `mainブランチにいることを確認します。`
Gemini: `ブランチ 'bump-1.2.3' を作成します。`
Gemini: `buildspec.json を更新しました。プルリクエストを作成します。`
Gemini: `プルリクエストのURL: https://github.com/kaito-tokyo/obs-showdraw/pull/123`
Gemini: `このプルリクエストをマージし、完了したら私に報告してください。`
User: `マージしました`
User: `タグをプッシュして`
Gemini: `バージョン 1.2.3 の Git タグをプッシュします。よろしいですか？`
User: `はい`
Gemini: `Git タグ '1.2.3' をプッシュしました。GitHub Actions でリリースワークフローが開始されます。`
Gemini: `GitHub リリースドラフトのURL: https://github.com/kaito-tokyo/obs-showdraw/releases/tag/1.2.3`
Gemini: `GitHub上でリリースを完了してください。`