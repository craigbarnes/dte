dte Release Checklist
=====================

1. Create release commit
   1. Update summary of changes in `CHANGELOG.md`
   2. Add link to release tarball and GPG signature in `CHANGELOG.md`
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
   3. Copy generated tarball to `public/dist/dte/` in [releases repo]
   4. Run `make generate` in [releases repo]
   5. Commit tarball and generated signature/checksums to [releases repo]
   6. Push [releases repo] and wait for [GitLab Pages job] to finish
   7. Check tarball link in `README.md` works
   8. Push tag to remotes

3. Create post-release commit
   1. Reset `VERSION` and `CFLAGS`
   2. `git commit -m 'Post-release updates'`

4. Create [GitLab release] and [GitHub release]
5. Update [portable builds] in `CHANGELOG.md`


[releases repo]: https://gitlab.com/craigbarnes/craigbarnes.gitlab.io/-/tree/master/public/dist/dte
[GitLab Pages job]: https://gitlab.com/craigbarnes/craigbarnes.gitlab.io/-/pipelines
[GitLab release]: https://gitlab.com/craigbarnes/dte/-/releases
[GitHub release]: https://github.com/craigbarnes/dte/releases
[portable builds]: https://gitlab.com/craigbarnes/dte/-/blob/master/CHANGELOG.md#portable-builds-for-linux
