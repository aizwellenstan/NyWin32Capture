#pragma once
#include <exception>
#include <vector>
#include <windows.h>
#include <strmif.h>
#include <amvideo.h>
#include <dshow.h>
#include <uuids.h>

#define NyWin32Capture_EXPORTS
#ifdef NyWin32Capture_EXPORTS
#define NyWin32Capture_API __declspec(dllexport)
#else
#define NyWin32Capture_API __declspec(dllimport)
#endif

namespace NyWin32Capture
{
	/**	���̃N���X�́ANyWin32CaptureException�̗�O���`���܂��B
	*/
	class NyWin32CaptureException :public std::exception
	{
	public:
		NyWin32CaptureException();
		NyWin32CaptureException(exception e);
		NyWin32CaptureException(const char* m);
	};
}

namespace NyWin32Capture
{
	/**	���̃N���X�́A1�̃r�f�I�t�H�[�}�b�g���i�[���A�e�l�ւ̃A�N�Z�X���񋟂��܂��B
	*/
	class VideoFormat
	{
	private:
		//�R�s�[�R���X�g���N�^�폜
		VideoFormat(const VideoFormat& );
		VideoFormat& operator=(const VideoFormat&);
	private:
		AM_MEDIA_TYPE *_pmt;
		VIDEO_STREAM_CONFIG_CAPS* _scc;
	public:
		VideoFormat(AM_MEDIA_TYPE* i_pmt,const VIDEO_STREAM_CONFIG_CAPS& i_scc);
		virtual ~VideoFormat();
		int getWidth()const;
		int getHeight()const;
		double getRate()const;
		const GUID& getFormat()const;
		const AM_MEDIA_TYPE* VideoFormat::getMediaType()const;
		const VIDEOINFOHEADER* getVideoInfoHeader()const;
		const BITMAPINFOHEADER* getBitmapInfoHeader()const;
	};
}

namespace NyWin32Capture
{
	class VideoFormatList
	{
	private:
		//�R�s�[�R���X�g���N�^�폜
		VideoFormatList(const VideoFormatList& );
		VideoFormatList& operator=(const VideoFormatList&);
	private:
		std::vector<VideoFormat*> *_list;
		void clear();
	public:
		VideoFormatList();
		void update(IAMStreamConfig* i_config);
		virtual ~VideoFormatList();
		/**	���̊֐��́A�w�肵���t�H�[�}�b�g�ɍ��v����VideoFormat�����X�g���猟�����܂��B
			����������΁A���̃t�H�[�}�b�g�ւ̃|�C���^��Ԃ��܂��B
			������Ȃ���΁ANULL��Ԃ��܂��B
		*/
		const VideoFormat* getFormat(int i_width,int i_height,const GUID& i_format)const;
		const VideoFormat* getFormat(int i_index)const;
		int getNumberOfFormat()const;
	};
}


struct ISampleGrabber;


namespace NyWin32Capture
{
	/**	���̃N���X�́A�L���v�`���f�o�C�X�P�𐧌䂵�܂��B
		���̃N���X�͂R�̏��ST_RUN,ST_IDLE,ST_CLOSED�������܂��B
		�X�e�[�^�X�ƁA�����ω�������֐����̊֌W�͈ȉ��̒ʂ�ł��B

		
		|function name      | status transition
		|-------------------+---------------------
		|openDevice         | ST_CLOSED -> ST_IDLE
		|(setting functions)| -
		|startDevice        | ST_IDLE   -> ST_RUN
		|(capture funstion )| -
		|stopDevice         | ST_RUN    -> ST_IDLE
		|closeDevice        | ST_IDLE   -> ST_CLOSED

	*/
	class CaptureDevice
	{
	private:
		const static int ST_CLOSED=0;
		const static int ST_IDLE  =1;
		const static int ST_RUN=2;
	private:
		IMoniker*    _moniker;
		BITMAPINFOHEADER _capture_format;
		struct{
			struct{
				IBaseFilter* filter;
				IAMStreamConfig* config;
				GUID pin_category;
			}sorce;
			struct{
				IMediaControl* mc;
				IGraphBuilder* graph;
			}graph_builder;
			ICaptureGraphBuilder2* cap_builder;
			struct{
				IBaseFilter* filter;
				ISampleGrabber* grab;
			}render;
		}ds_res;
		struct{
			WCHAR* name;
		}_allocated_res;
		int _status;
	public:
		CaptureDevice(IMoniker* i_moniker);
		virtual ~CaptureDevice();
		void startCapture();
		void stopCapture();
		void openDevice();
		void closeDevice();
		/** �f�o�C�X�̒񋟂ł���r�f�I�t�H�[�}�b�g�̈ꗗ���Ao_list�����ɕԂ��܂��B
			���̊֐��́AST_IDLE,ST_RUN�X�e�[�^�X�̂Ƃ������g�p�\�ł��B
		*/
		void getVideoFormatList(VideoFormatList& o_list)const;
		/** �L���v�`���C���[�W���擾���܂��B
			���̊֐��́AST_RUN�X�e�[�^�X�̂Ƃ������g�p�\�ł��B
		*/
		bool captureImage(void* i_buf,long i_buf_size=0);
		/** �L���v�`���C���[�W�̃t�H�[�}�b�g���w�肵�܂��B
			���̊֐��́AST_IDLE�X�e�[�^�X�̂Ƃ������g�p�\�ł��B
		*/
		bool setVideoFormat(int i_width,int i_height,GUID i_format,double i_rate);
		/** �L���v�`���C���[�W�̃t�H�[�}�b�g���w�肵�܂��B
			���̊֐��́AST_IDLE�X�e�[�^�X�̂Ƃ������g�p�\�ł��B
		*/
		bool setVideoFormat(const VideoFormat& i_format,double i_rate);
		/**	�L���v�`���摜�̃t�H�[�}�b�g���擾���܂��B
			���̊֐��́AST_RUN�X�e�[�^�X�̂Ƃ������g�p�\�ł��B
		*/
		const BITMAPINFOHEADER& getImageFormat()const;
		const WCHAR* getName()const;
	};
}

namespace NyWin32Capture
{
	class CaptureDeviceList
	{
	private:
		void createDeviceList();
		void releaseDeviceList();
		std::vector<CaptureDevice*> *_list;
	public:
		CaptureDeviceList();
		virtual ~CaptureDeviceList();
		//	i_index�Ԗڂ̃L���v�`���f�o�C�X���擾���܂��B
		CaptureDevice* getDevice(int i_index)const;
		//	�f�o�C�X�̐����擾���܂��B
		int getNumberOfDevice()const;
	};
}