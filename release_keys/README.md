# Release keys

`wfc-release.public` is the public half of the WPM signing key for WFC
packages. The CI `package` job (version tags only) trusts it to verify the
packages it signs. The private half lives only in the GitHub secret
`WPM_RELEASE_PRIVATE_KEY` of the protected `release` environment; it is never
committed or printed.

To create the pair (the owner does this; WCRT's key is a separate key):

```powershell
wpm keygen wfc-release.private wfc-release.public
```

1. Commit `wfc-release.public` here.
2. Store the private file as the environment secret, then delete the local copy:
   `gh secret set WPM_RELEASE_PRIVATE_KEY --env release < wfc-release.private`
3. Protect the `release` environment (required reviewers) in the repository
   settings.
