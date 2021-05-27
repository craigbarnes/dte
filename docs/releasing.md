dte Release Checklist
=====================

1. Create release commit
   1. Update summary of changes in `CHANGELOG.md`
   2. Add link to release tarball in `CHANGELOG.md`
   3. Update tarball name in `README.md`
   4. Hard code `VERSION` variable in `mk/build.mk` to release version
   5. Update `RELEASE_VERSIONS` in `mk/dev.mk`
   6. Remove `-g` from default `CFLAGS`
   7. Check `make vars` output
   8. `git commit -m "Prepare v${VER} release"`

2. Tag and upload
   1. `git tag -s -m "Release v${VER}" v${VER} ${COMMIT}`
   2. `make dist`
   3. Upload tarball and detached GPG signature to GitLab pages
   4. Wait for GitLab pages job to finish
   5. Check tarball link in `README.md` works
   6. Push tag to remotes

3. Create post-release commit
   1. Add link to GPG signature in `CHANGELOG.md`
   2. Update `mk/sha256sums.txt`
   3. Reset `VERSION` and `CFLAGS`
