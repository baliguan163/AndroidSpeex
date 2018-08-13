
#include <jni.h>

#include <string.h>
#include <unistd.h>

#include <speex/speex.h>

static int codec_open = 0;

static int dec_frame_size;
static int enc_frame_size;

static SpeexBits ebits, dbits;
void *enc_state;
void *dec_state;
static JavaVM *gJavaVM;


//一：编码流程
//使用Speex的API函数对音频数据进行压缩编码要经过如下步骤：
//1、定义一个SpeexBits类型变量bits和一个Speex编码器状态变量enc_state。
//2、调用speex_bits_init(&bits)初始化bits。
//3、调用speex_encoder_init(&speex_nb_mode)来初始化enc_state。其中speex_nb_mode是SpeexMode类型的变量，表示的是窄带模式。还有speex_wb_mode表示宽带模式、speex_uwb_mode表示超宽带模式。
//4、调用函数int speex_encoder_ ctl(void *state, int request, void *ptr)来设定编码器的参数，其中参数state表示编码器的状态；参数request表示要定义的参数类型，如SPEEX_ GET_ FRAME_SIZE表示设置帧大小，SPEEX_ SET_QUALITY表示量化大小，这决定了编码的质量；参数ptr表示要设定的值。
//可通过speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &frame_size) 和speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality)来设定编码器的参数。
//5、初始化完毕后，对每一帧声音作如下处理：调用函数speex_bits_reset(&bits)再次设定SpeexBits，然后调用函数speex_encode(enc_state, input_frame, &bits)，参数bits中保存编码后的数据流。
//6、编码结束后，调用函数speex_bits_destroy (&bits)，    speex_encoder_destroy (enc_state)来
//二：解码流程
//同样，对已经编码过的音频数据进行解码要经过以下步骤：
//1、     定义一个SpeexBits类型变量bits和一个Speex编码状态变量enc_state。
//2、   调用speex_bits_init(&bits)初始化bits。
//3、   调用speex_decoder_init (&speex_nb_mode)来初始化enc_state。
//4、    调用函数speex_decoder_ctl (void *state, int request, void *ptr)来设定编码器的参数。
//5、   调用函数 speex_decode(void *state, SpeexBits *bits, float *out)对参数bits中的音频数据进行解编码，参数out中保存解码后的数据流。
//6、   调用函数speex_bits_destroy(&bits), speex_ decoder_ destroy (void *state)来关闭和销毁SpeexBits和解码器。

//还可以能过下面函数设置是否使用“知觉增强”功能
//speex_decoder_ctl( dec_state, SPEEX_SET_ENH, &enh );
//如果enh是0则表是不启用，1则表示启用。在1.2-beta1中，默认是开启的。

//open()方法中需要传一个压缩质量（quality是一个0～10（包含10）范围内的整数）的参数；
extern "C"
JNIEXPORT jint JNICALL Java_com_audio_lib_SpeexUtil_open
  (JNIEnv *env, jobject obj, jint compression) {
	int tmp;
	if (codec_open++ != 0)
		return (jint)0;

	speex_bits_init(&ebits);
	speex_bits_init(&dbits);
//	调用speex_encoder_init(&speex_nb_mode)来初始化enc_state。
//			其中speex_nb_mode是SpeexMode类型的变量，表示的是窄带模式。
//			还有speex_wb_mode表示宽带模式、
//			speex_uwb_mode表示超宽带模式。
	enc_state = speex_encoder_init(&speex_nb_mode);
	dec_state = speex_decoder_init(&speex_nb_mode);
	tmp = compression;
	speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &tmp);
	speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &enc_frame_size);
	speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &dec_frame_size);
	return (jint)0;
}


//encode()方法中有short lin[], int offset, byte encoded[], int size四个参数，
//		其中short lin[]表示录音得到的short型数据，
//		int offset为跳过的字节数，
//		byte encoded[]为压缩后的byte型数据，
//		int size为数据的长度；
extern "C"
JNIEXPORT jint JNICALL Java_com_audio_lib_SpeexUtil_encode
    (JNIEnv *env, jobject obj,
    		jshortArray lin, jint offset, jbyteArray encoded, jint size) {

        jshort buffer[enc_frame_size];
        jbyte output_buffer[enc_frame_size];
	int nsamples = (size-1)/enc_frame_size + 1;
	int i, tot_bytes = 0;

	if (!codec_open)
		return 0;

	speex_bits_reset(&ebits);

	for (i = 0; i < nsamples; i++) {
		env->GetShortArrayRegion(lin, offset + i*enc_frame_size, enc_frame_size, buffer);
		speex_encode_int(enc_state, buffer, &ebits);
	}

	tot_bytes = speex_bits_write(&ebits, (char *)output_buffer,
				     enc_frame_size);
	env->SetByteArrayRegion(encoded, 0, tot_bytes,
				output_buffer);

        return (jint)tot_bytes;
}


//decode()方法中有byte encoded[], short lin[], int size三个参数，
//		其中byte encoded[]表示压缩后的byte型数据，
//		short lin[]为解压后的short型数据，
//		int size为数据大小；
extern "C"
JNIEXPORT jint JNICALL Java_com_audio_lib_SpeexUtil_decode
    (JNIEnv *env, jobject obj, jbyteArray encoded, jshortArray lin, jint size) {

        jbyte buffer[dec_frame_size];
        jshort output_buffer[dec_frame_size];
        jsize encoded_length = size;

	if (!codec_open)
		return 0;

	env->GetByteArrayRegion(encoded, 0, encoded_length, buffer);
	speex_bits_read_from(&dbits, (char *)buffer, encoded_length);
	speex_decode_int(dec_state, &dbits, output_buffer);
	env->SetShortArrayRegion(lin, 0, dec_frame_size,output_buffer);
	return (jint)dec_frame_size;
}

//getFrameSize()方法获得帧字节的大小；
extern "C"
JNIEXPORT jint JNICALL Java_com_audio_lib_SpeexUtil_getFrameSize(JNIEnv *env, jobject obj) {
	if (!codec_open)
		return 0;
	return (jint)enc_frame_size;

}

//close()方法退出语音压缩和解压。
extern "C"
JNIEXPORT void JNICALL Java_com_audio_lib_SpeexUtil_close(JNIEnv *env, jobject obj) {
	if (--codec_open != 0)
		return;

	speex_bits_destroy(&ebits);
	speex_bits_destroy(&dbits);
	speex_decoder_destroy(dec_state);
	speex_encoder_destroy(enc_state);
}
