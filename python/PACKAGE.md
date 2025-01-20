Install build system
```sh
python3 -m pip install --upgrade build
```

Generate distribution files
```sh
python3 -m build
```

Install deployment system
```sh
python3 -m pip install --upgrade twine
```

Deploy to test repository
```sh
python3 -m twine upload --repository testpypi dist/*
```