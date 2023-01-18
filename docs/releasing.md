dte Release Checklist
=====================

1. Create release commit
   1. Update summary of changes in `CHANGELOG.md`
   2. Add link to release tarball in `CHANGELOG.md`
   3. Update tarball name in `README.md`
   4. Hard code `VERSION` variable in `mk/build.mk` to release version
   5. Update `RELEASE_VERSIONS` in `mk/dev.mk`
   5. Update `<releases>` in `dte.appdata.xml`
   6. Remove `-g` from default `CFLAGS`
   7. Check `make vars` output
   8. `git commit -m "Prepare v${VER} release"`

2. Tag and upload
   1. `git tag -s -m "Release v${VER}" v${VER} ${COMMIT}`
   2. `make dist`
   3. Upload tarball and detached GPG signature to [releases repo]
   4. Wait for [GitLab Pages job] to finish
   5. Check tarball link in `README.md` works
   6. Push tag to remotes

3. Create post-release commit
   1. Add link to GPG signature in `CHANGELOG.md`
   2. Reset `VERSION` and `CFLAGS`
   3. `git commit -m 'Post-release updates'`

4. Update "portable builds" in `CHANGELOG.md`


[releases repo]: https://gitlab.com/craigbarnes/craigbarnes.gitlab.io/-/tree/master/public/dist/dte
[GitLab Pages job]: https://gitlab.com/craigbarnes/craigbarnes.gitlab.io/-/pipelines
