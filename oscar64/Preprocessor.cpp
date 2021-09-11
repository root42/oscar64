#include "Preprocessor.h"
#include <string.h>

SourcePath::SourcePath(const char* path)
{
	strcpy_s(mPathName, path);
}

SourcePath::~SourcePath(void)
{

}

bool SourceFile::ReadLine(char* line)
{
	if (mFile)
	{
		if (fgets(line, 1024, mFile))
			return true;

		fclose(mFile);
		mFile = nullptr;
	}

	return false;
}

SourceFile::SourceFile(void) 
	: mFile(nullptr), mFileName{ 0 }
{

}

SourceFile::~SourceFile(void)
{
	if (mFile)
	{
		fclose(mFile);
		mFile = nullptr;
	}
}

bool SourceFile::Open(const char* name, const char* path)
{
	strcpy_s(mFileName, path);
	int	n = strlen(mFileName);

	if (n > 0 && mFileName[n - 1] != '/')
	{
		mFileName[n++] = '/';
		mFileName[n] = 0;
	}

	strcat_s(mFileName + n, sizeof(mFileName) - n, name);

	if (!fopen_s(&mFile, mFileName, "r"))
		return true;

	return false;
}

void SourceFile::Close(void)
{
	if (mFile)
	{
		fclose(mFile);
		mFile = nullptr;
	}
}

bool Preprocessor::NextLine(void)
{
	int	s = 0;
	while (mSource->ReadLine(mLine + s))
	{
		mLocation.mLine++;

		s = strlen(mLine);
		while (s > 0 && mLine[s - 1] == '\n')
			s--;
		if (s == 0 || mLine[s - 1] != '\\')
			return true;
		s--;
	}

	return false;
}

bool Preprocessor::OpenSource(const char* name, bool local)
{
	if (mSource)
		mSource->mLocation = mLocation;

	SourceFile	*	source = new SourceFile();

	bool	ok = false;

	if (source->Open(name, ""))
		ok = true;

	if (!ok && local && mSource)
	{
		char	lpath[200];
		strcpy_s(lpath, mSource->mFileName);
		int	i = strlen(lpath);
		while (i > 0 && lpath[i - 1] != '/')
			i--;
		lpath[i] = 0;

		if (source->Open(name, lpath))
			ok = true;
	}

	SourcePath* p = mPaths;
	while (!ok && p)
	{
		if (source->Open(name, p->mPathName))
			ok = true;
		else
			p = p->mNext;
	}
	
	if (ok)
	{
		printf("Reading %s\n", source->mFileName);
		source->mUp = mSource;
		mSource = source;
		mLocation.mFileName = mSource->mFileName;
		mLocation.mLine = 0;
		mLine[0] = 0;

		return true;
	}
	else
		return false;
}

bool Preprocessor::CloseSource(void)
{
	if (mSource)
	{
		mSource = mSource->mUp;
		if (mSource)
		{
			mLocation = mSource->mLocation;
			mLine[0] = 0;
			return true;
		}
	}
	
	return false;
}

Preprocessor::Preprocessor(Errors* errors)
	: mSource(nullptr), mSourceList(nullptr), mPaths(nullptr), mErrors(errors)
{

}

Preprocessor::~Preprocessor(void)
{
}

void Preprocessor::AddPath(const char* path)
{
	SourcePath* sp = new SourcePath(path);
	sp->mNext = mPaths;
	mPaths = sp;
}
