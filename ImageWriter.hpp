#ifndef IMAGE_WRITER_HPP
#define IMAGE_WRITER_HPP

#include "Platform.hpp"
#include "MurmurHash3.hpp"
#include "ImageReader.hpp"
#include "EndianNeutral.hpp"

namespace cat {


// When API functions return an int, it's all about this:
enum WriteErrors {
	WE_OK,			// No error

	WE_BAD_DIMS,	// Image dimensions are invalid
	WE_FILE,		// Unable to access file

	WE_COUNT
};


//// WriteVector

/*
 * Vector optimized for file write access pattern:
 *
 * + Only writes are to append
 * + Final operation is to read it all back out
 *
 * Allocates data in ropes that double in size
 * Each rope ends in a pointer to the next rope
 *
 * Cannot just memory map a file and append to it because mmap files cannot
 * grow at all.  So my solution is this optimal vector representation and
 * then write it all out.  Data is stored internally in little-endian byte
 * order so that it can just be memcpy out to the file.
 *
 * Iunno...  Speeeed! -cat
 */

class WriteVector {
	static const int HEAD_SIZE = 128; // Words in head
	static const int WORD_BYTES = static_cast<int>( sizeof(u32) );
	static const int PTR_BYTES = static_cast<int>( sizeof(u32 *) );
	static const int PTR_WORDS = PTR_BYTES / WORD_BYTES;

	u32 *_head; 	// First rope strand
	u32 *_work; 	// Rope under construction
	int _used;		// Words used in workspace
	int _allocated;	// Words allocated in workspace

	int _size;		// Total number of words

	MurmurHash3 _hash;

	void clear();
	void grow();

public:
	CAT_INLINE WriteVector() {
		_head = _work = 0;
		_used = _allocated = _size = 0;
	}

	CAT_INLINE virtual ~WriteVector() {
		clear();
	}

	void init(u32 hashSeed);

	CAT_INLINE void push(u32 x) {
		// Grow ropes
		if CAT_UNLIKELY(_used >= _allocated) {
			grow();
		}

		// Munge and write data
		_hash.hashWord(x);
		_work[_used++] = getLE(x);
		++_size;
	}

	CAT_INLINE u32 finalizeHash() {
		return _hash.final(_size);
	}

	CAT_INLINE int getWordCount() {
		return _size;
	}

	void write(u32 *target);
};


//// ImageWriter

class ImageWriter {
	ImageInfo _info;

	WriteVector _words;
	u32 _work;	// Word workspace
	int _bits;	// Modulo 32

public:
	CAT_INLINE ImageWriter() {
	}
	CAT_INLINE virtual ~ImageWriter() {
	}

	CAT_INLINE ImageInfo *getImageInfo() {
		return &_info;
	}

	int init(int width, int height);

	// Only works for 1-bit code, and code must not have dirty high bits
	void writeBit(u32 code);

	// Only works with len in [1..32], and code must not have dirty high bits
	void writeBits(u32 code, int len);

	// Write a whole 32-bit word at once
	void writeWord(u32 word);

	int finalizeAndWrite(const char *path);
};


} // namespace cat

#endif // IMAGE_WRITER_HPP

