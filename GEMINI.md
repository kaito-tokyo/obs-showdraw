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

To initiate a new release, the user will instruct Gemini to start the process (e.g., "リリースを開始して"). Gemini will then perform the following steps:

1.  **Specify New Version**:
    *   **ACTION**: Display the current version.
    *   **ACTION**: Prompt the user to provide the new version number (e.g., `1.0.0`, `1.0.0-beta1`).
    *   **CONSTRAINT**: The version must follow Semantic Versioning (e.g., `MAJOR.MINOR.PATCH`).

2.  **Prepare & Update `buildspec.json`**:
    *   **ACTION**: Confirm the current branch is `main` and synchronized with the remote.
    *   **ACTION**: Create a new branch (`bump-X.Y.Z`).
    *   **ACTION**: Update the `version` field in `buildspec.json`.

3.  **Create & Merge Pull Request (PR)**:
    *   **ACTION**: Create a PR for the version update.
    *   **ACTION**: Provide the URL of the created PR.
    *   **ACTION**: Instruct the user to merge this PR.
    *   **PAUSE**: Wait for user confirmation of PR merge.

4.  **Push Git Tag**:
    *   **TRIGGER**: User instructs Gemini to push the Git tag after PR merge confirmation.
    *   **ACTION**: Switch to the `main` branch.
    *   **ACTION**: Synchronize with the remote.
    *   **ACTION**: Verify the `buildspec.json` version.
    *   **ACTION**: Push the Git tag.
    *   **CONSTRAINT**: The tag must be `X.Y.Z` (no 'v' prefix).
    *   **RESULT**: Pushing the tag triggers the automated release workflow.

5.  **Finalize Release**:
    *   **ACTION**: Provide the draft release URL.
    *   **INSTRUCTION**: User completes the release on GitHub.

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