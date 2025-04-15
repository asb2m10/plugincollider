all: noodle.sc tweet2syndef.sc
	sclang noodle.sc
	sclang tweet2syndef.sc

tweet2syndef.sc: ../../resources/tests/tweets_collection.sc
	python tweet2syndef.py

clean:
	rm *.scsyndef
	rm tweet2syndef.sc