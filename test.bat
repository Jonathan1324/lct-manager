@echo off

python -m ci.ci -d

.\dist\bin\lct %*
