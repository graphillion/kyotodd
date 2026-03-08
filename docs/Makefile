.PHONY: html clean doxygen

doxygen:
	doxygen Doxyfile

html: doxygen
	sphinx-build -b html . _build/html

clean:
	rm -rf _build _doxygen
