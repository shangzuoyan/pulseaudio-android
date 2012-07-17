#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <system/audio.h>
#include <media/AudioTrack.h>

#include <pulse/pulseaudio.h>

#if 0
#define DEBUG 1
#define DEBUG_TIMING 1
#endif

#if 0
#define LATENCY_TEST 1
#endif

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#define ERROR_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...) do { } while (0)
#define ERROR_PRINT(...) do { } while (0)
#endif

#ifdef DEBUG_TIMING
#include <sys/time.h>
#endif

using namespace android;

struct userdata {
	int fd;
	sem_t sem;

	int rate, latency;

	AudioTrack *at;

	pa_threaded_mainloop *ml;
	pa_context *c;
	pa_stream *s;

	volatile int play;
} ud;

void _af_cb(int event, void *user, void *info)
{
	struct userdata *u = (struct userdata *) user;

	switch (event) {
		case AudioTrack::EVENT_MORE_DATA:
		{
			int ret;
			AudioTrack::Buffer *buf = (AudioTrack::Buffer *) info;
#ifdef DEBUG_TIMING
			static struct timeval tv1, tv2;
#endif

			if (ud.play)
				ret = read(u->fd, buf->raw, buf->size);
			else {
				memset(buf->raw, 0, buf->size);
				ret = buf->size;
			}

			if (ret < 0) {
				perror("Read failed:");
				exit(EXIT_FAILURE);
			}

			buf->size = ret;

			if (ret == 0) {
				sem_post(&u->sem);
				return;
			}

#ifdef DEBUG_TIMING
			gettimeofday(&tv2, NULL);
			DEBUG_PRINT("[%llu] ", (unsigned long long)
				((tv2.tv_sec * 1000000L) + tv2.tv_usec) -
				((tv1.tv_sec * 1000000L) + tv1.tv_usec));
			tv1 = tv2;
#endif

			DEBUG_PRINT("More data (%d)\n", buf->size);

			break;
		}

		case AudioTrack::EVENT_UNDERRUN:
			DEBUG_PRINT("Underrun\n");
			break;

		case AudioTrack::EVENT_LOOP_END:
			DEBUG_PRINT("Loop end\n");
			break;

		case AudioTrack::EVENT_MARKER:
			DEBUG_PRINT("Marker\n");
			break;

		case AudioTrack::EVENT_NEW_POS:
			DEBUG_PRINT("New pos\n");
			break;

		case AudioTrack::EVENT_BUFFER_END:
			DEBUG_PRINT("Buffer end\n");
			break;
	}
}

void _pa_stream_write_cb(pa_stream *s, size_t size, void *user)
{
	static int once = 0;
	struct userdata *u = (struct userdata *) user;
	size_t m_size, orig_size = size;
	int ret;
	void *m_buf;
#ifdef DEBUG_TIMING
	static struct timeval tv1, tv2;
#endif

	if (!once) {
		pa_usec_t l;

		if (pa_stream_get_latency(s, &l, NULL) == 0) {
			printf("Total latency: %llu\n", (unsigned long long) l);
			once = 1;
		}
	}


	while (size) {
		m_size = size;
		if (pa_stream_begin_write(s, &m_buf, &m_size) < 0) {
			ERROR_PRINT("Begin write failed\n");
			exit(EXIT_FAILURE);
		}

		if (ud.play)
			ret = read(u->fd, m_buf, m_size);
		else {
			memset(m_buf, 0, m_size);
			ret = m_size;
		}

		if (ret < 0) {
			perror("Read failed:");
			exit(EXIT_FAILURE);
		}

		if (ret == 0) {
			sem_post(&u->sem);
			return;
		}

		m_size = ret;
		size -= m_size;
		pa_stream_write(s, m_buf, m_size, NULL, 0, PA_SEEK_RELATIVE);

		DEBUG_PRINT("wrote %u\n", m_size);
	}

#ifdef DEBUG_TIMING
	gettimeofday(&tv2, NULL);
	DEBUG_PRINT("[%llu] ", (unsigned long long)
			((tv2.tv_sec * 1000000L) + tv2.tv_usec) -
			((tv1.tv_sec * 1000000L) + tv1.tv_usec));
	tv1 = tv2;
#endif

	DEBUG_PRINT("More data (%d)\n", orig_size);
}

void _pa_context_cb(pa_context *c, void *user)
{
	struct userdata *u = (struct userdata *) user;

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_READY: {
			pa_sample_spec ss;
			pa_buffer_attr attr = { -1, -1, -1, -1, -1 };

			ss.format = PA_SAMPLE_S16LE;
			ss.rate = u->rate;
			ss.channels = 2;

			u->s = pa_stream_new(c, "Playback test", &ss, NULL);

			if (u->latency > 0)
				attr.tlength = (pa_bytes_per_second(&ss) * u->latency) / 1000;

			pa_stream_set_write_callback(u->s, _pa_stream_write_cb, u);

			if (pa_stream_connect_playback(u->s,
						NULL,
						&attr,
						(pa_stream_flags_t) (PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE | PA_STREAM_START_UNMUTED),
						NULL,
						NULL) < 0) {
				ERROR_PRINT("Error creating stream\n");
				exit(EXIT_FAILURE);
			}

			break;
		}

		case PA_CONTEXT_FAILED:
			exit(EXIT_FAILURE);
			break;

		case PA_CONTEXT_TERMINATED:
			exit(EXIT_SUCCESS);
			break;

		default:
			break;
	}
}

int main(int argc, char **argv)
{
	enum {
		AUDIOFLINGER,
		PULSEAUDIO,
	};

	int opt;
	int backend = AUDIOFLINGER;
	status_t ret;

	struct option long_opts[] = {
		{ "rate", required_argument, NULL, 'r' },
		{ "latency", required_argument, NULL, 'l' },
		{ "backend", required_argument, NULL, 'b' },
		{ 0, 0, 0, 0},
	};

	if (argc < 2) {
		ERROR_PRINT("-ENOTENOUGHARGUMENTS\n");
		return -1;
	}

	ud.rate = 44100;
	ud.latency = 0;

	while ((opt = getopt_long(argc, argv, "r:l:b:", long_opts, NULL)) != -1) {
		switch (opt) {
			case 'r':
				errno = 0;
				ud.rate = strtol(optarg, NULL, 0);

				if (errno || !ud.rate) {
					ERROR_PRINT("Bad rate\n");
					return -1;
				}

				break;

			case 'l':
				errno = 0;
				ud.latency = strtol(optarg, NULL, 0);

				if (errno || !ud.latency) {
					ERROR_PRINT("Bad latency\n");
					return -1;
				}

				break;

			case 'b':
				if (strcmp(optarg, "af") == 0)
					backend = AUDIOFLINGER;
				else if (strcmp(optarg, "pa") == 0)
					backend = PULSEAUDIO;
				else {
					ERROR_PRINT("Invalid backend\n");
					return -1;
				}

				break;

			default:
				ERROR_PRINT("Bad arguments");
				return -1;
		}
	}

	if (optind != argc -1) {
		ERROR_PRINT("Bad argument count\n");
		return -1;
	}

	ud.fd = open(argv[optind], O_RDONLY);
	if (ud.fd < 0) {
		perror("Error opening file:");
		return -1;
	}

	printf("Rate: %d\n", ud.rate);
	printf("Req. latency: %d\n", ud.latency);

	sem_init(&ud.sem, 0, 0);

	if (backend == AUDIOFLINGER) {
		ud.at = new AudioTrack(AUDIO_STREAM_MUSIC,
				ud.rate,
				AUDIO_FORMAT_PCM_16_BIT,
				AUDIO_CHANNEL_OUT_STEREO,
				ud.latency * (ud.rate / 1000),
				0,
				_af_cb,
				&ud);

		ret = ud.at->initCheck();
		if (ret != NO_ERROR) {
			ERROR_PRINT("Init failed, wtf. %d\n", ret);
			return -1;
		}

		ud.at->start();

		printf("Total latency: %u\n", ud.at->latency());
	} else {
		ud.ml = pa_threaded_mainloop_new();
		ud.c = pa_context_new(pa_threaded_mainloop_get_api(ud.ml), "Playback test");

		pa_context_set_state_callback(ud.c, _pa_context_cb, &ud);

		if (pa_context_connect(ud.c, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
			ERROR_PRINT("Failed to connect context");
			return -1;
		}

		pa_threaded_mainloop_start(ud.ml);
	}

#ifdef LATENCY_TEST
	ud.play = 0;
	fgetc(stdin);
	ud.play = 1;
	fgetc(stdin);
	ud.play = 0;
	fgetc(stdin);
	ud.play = 1;
	fgetc(stdin);
	ud.play = 0;
	fgetc(stdin);
#endif
	ud.play = 1;

	sem_wait(&ud.sem);

	if (backend == AUDIOFLINGER) {
		ud.at->stop();
	} else {
		pa_stream_set_write_callback(ud.s, NULL, NULL);
		pa_stream_disconnect(ud.s);
		pa_stream_unref(ud.s);

		pa_context_set_state_callback(ud.c, NULL, NULL);
		pa_context_disconnect(ud.c);
		pa_context_unref(ud.c);

		pa_threaded_mainloop_stop(ud.ml);
		pa_threaded_mainloop_free(ud.ml);
	}

	close(ud.fd);
	sem_destroy(&ud.sem);

	return 0;
}
