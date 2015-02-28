#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <check.h>
#include <stdlib.h>
#include <time.h>

#include "../toxcore/tox.h"

#include "helpers.h"

#if defined(_WIN32) || defined(__WIN32__) || defined (WIN32)
#define c_sleep(x) Sleep(1*x)
#else
#include <unistd.h>
#define c_sleep(x) usleep(1000*x)
#endif

void accept_friend_request(Tox *m, const uint8_t *public_key, const uint8_t *data, size_t length, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    if (length == 7 && memcmp("Gentoo", data, 7) == 0) {
        tox_friend_add_norequest(m, public_key, 0);
    }
}
uint32_t messages_received;

void print_message(Tox *m, uint32_t friendnumber, const uint8_t *string, size_t length, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    uint8_t cmp_msg[TOX_MAX_MESSAGE_LENGTH];
    memset(cmp_msg, 'G', sizeof(cmp_msg));

    if (length == TOX_MAX_MESSAGE_LENGTH && memcmp(string, cmp_msg, sizeof(cmp_msg)) == 0)
        ++messages_received;
}

uint32_t name_changes;

void print_nickchange(Tox *m, uint32_t friendnumber, const uint8_t *string, size_t length, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    if (length == sizeof("Gentoo") && memcmp(string, "Gentoo", sizeof("Gentoo")) == 0)
        ++name_changes;
}

uint32_t typing_changes;

void print_typingchange(Tox *m, uint32_t friendnumber, bool typing, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    if (!typing)
        typing_changes = 1;
    else
        typing_changes = 2;
}

uint32_t custom_packet;

void handle_custom_packet(Tox *m, uint32_t friend_num, const uint8_t *data, size_t len, void *object)
{
    uint8_t number = *((uint32_t *)object);

    if (len != TOX_MAX_CUSTOM_PACKET_SIZE)
        return;

    uint8_t f_data[len];
    memset(f_data, number, len);

    if (memcmp(f_data, data, len) == 0) {
        ++custom_packet;
    } else {
        printf("Custom packet fail. %u\n", number );
    }

    return;
}

uint8_t filenum;
uint32_t file_accepted;
uint64_t file_size;
void file_request_accept(Tox *m, int friendnumber, uint8_t filenumber, uint64_t filesize, const uint8_t *filename,
                         uint16_t filename_length, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    if (filename_length == sizeof("Gentoo.exe") && memcmp(filename, "Gentoo.exe", sizeof("Gentoo.exe")) == 0)
        ++file_accepted;

    file_size = filesize;
    tox_file_send_control(m, friendnumber, 1, filenumber, TOX_FILECONTROL_ACCEPT, NULL, 0);
}

uint32_t file_sent;
uint32_t sendf_ok;
void file_print_control(Tox *m, int friendnumber, uint8_t receive_send, uint8_t filenumber, uint8_t control_type,
                        const uint8_t *data, uint16_t length, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    if (receive_send == 0 && control_type == TOX_FILECONTROL_FINISHED)
        tox_file_send_control(m, friendnumber, 1, filenumber, TOX_FILECONTROL_FINISHED, NULL, 0);

    if (receive_send == 1 && control_type == TOX_FILECONTROL_FINISHED)
        file_sent = 1;

    if (receive_send == 1 && control_type == TOX_FILECONTROL_ACCEPT)
        sendf_ok = 1;

}

uint64_t size_recv;
uint8_t num;
void write_file(Tox *m, int friendnumber, uint8_t filenumber, const uint8_t *data, uint16_t length, void *userdata)
{
    if (*((uint32_t *)userdata) != 974536)
        return;

    uint8_t f_data[length];
    memset(f_data, num, length);
    ++num;

    if (memcmp(f_data, data, length) == 0) {
        size_recv += length;
    } else {
        printf("FILE_CORRUPTED\n");
    }
}

START_TEST(test_one)
{
    Tox *tox1 = tox_new(0, 0, 0, 0);
    Tox *tox2 = tox_new(0, 0, 0, 0);

    uint8_t address[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox1, address);
    TOX_ERR_FRIEND_ADD error;
    uint32_t ret = tox_friend_add(tox1, address, (uint8_t *)"m", 1, &error);
    ck_assert_msg(ret == UINT32_MAX && error == TOX_ERR_FRIEND_ADD_OWN_KEY, "Adding own address worked.");

    tox_self_get_address(tox2, address);
    uint8_t message[TOX_MAX_FRIEND_REQUEST_LENGTH + 1];
    ret = tox_friend_add(tox1, address, NULL, 0, &error);
    ck_assert_msg(ret == UINT32_MAX && error == TOX_ERR_FRIEND_ADD_NULL, "Sending request with no message worked.");
    ret = tox_friend_add(tox1, address, message, 0, &error);
    ck_assert_msg(ret == UINT32_MAX && error == TOX_ERR_FRIEND_ADD_NO_MESSAGE, "Sending request with no message worked.");
    ret = tox_friend_add(tox1, address, message, sizeof(message), &error);
    ck_assert_msg(ret == UINT32_MAX && error == TOX_ERR_FRIEND_ADD_TOO_LONG,
                  "TOX_MAX_FRIEND_REQUEST_LENGTH is too big.");

    address[0]++;
    ret = tox_friend_add(tox1, address, (uint8_t *)"m", 1, &error);
    ck_assert_msg(ret == UINT32_MAX && error == TOX_ERR_FRIEND_ADD_BAD_CHECKSUM,
                  "Adding address with bad checksum worked.");

    tox_self_get_address(tox2, address);
    ret = tox_friend_add(tox1, address, message, TOX_MAX_FRIEND_REQUEST_LENGTH, &error);
    ck_assert_msg(ret == 0 && error == TOX_ERR_FRIEND_ADD_OK, "Failed to add friend.");
    ret = tox_friend_add(tox1, address, message, TOX_MAX_FRIEND_REQUEST_LENGTH, &error);
    ck_assert_msg(ret == UINT32_MAX && error == TOX_ERR_FRIEND_ADD_ALREADY_SENT, "Adding friend twice worked.");

    uint8_t name[TOX_MAX_NAME_LENGTH];
    int i;

    for (i = 0; i < TOX_MAX_NAME_LENGTH; ++i) {
        name[i] = rand();
    }

    tox_self_set_name(tox1, name, sizeof(name), 0);
    ck_assert_msg(tox_self_get_name_size(tox1) == sizeof(name), "Can't set name of TOX_MAX_NAME_LENGTH");

    size_t save_size = tox_save_size(tox1);
    uint8_t data[save_size];
    tox_save(tox1, data);

    tox_kill(tox2);
    TOX_ERR_NEW err_n;

    tox2 = tox_new(0, data, save_size, &err_n);
    ck_assert_msg(err_n == TOX_ERR_NEW_OK, "Load failed");

    ck_assert_msg(tox_self_get_name_size(tox2) == sizeof name, "Wrong name size.");

    uint8_t new_name[TOX_MAX_NAME_LENGTH] = { 0 };
    tox_self_get_name(tox2, new_name);
    ck_assert_msg(memcmp(name, new_name, TOX_MAX_NAME_LENGTH) == 0, "Wrong name");

    tox_kill(tox1);
    tox_kill(tox2);
}
END_TEST

START_TEST(test_few_clients)
{
    long long unsigned int con_time, cur_time = time(NULL);
    Tox *tox1 = tox_new(0, 0, 0, 0);
    Tox *tox2 = tox_new(0, 0, 0, 0);
    Tox *tox3 = tox_new(0, 0, 0, 0);
    ck_assert_msg(tox1 || tox2 || tox3, "Failed to create 3 tox instances");
    uint32_t to_compare = 974536;
    tox_callback_friend_request(tox2, accept_friend_request, &to_compare);
    uint8_t address[TOX_ADDRESS_SIZE];
    tox_self_get_address(tox2, address);
    uint32_t test = tox_friend_add(tox3, address, (uint8_t *)"Gentoo", 7, 0);
    ck_assert_msg(test == 0, "Failed to add friend error code: %i", test);

    uint8_t off = 1;

    while (1) {
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (tox_get_connection_status(tox1) && tox_get_connection_status(tox2) && tox_get_connection_status(tox3) && off) {
            printf("Toxes are online, took %llu seconds\n", time(NULL) - cur_time);
            con_time = time(NULL);
            off = 0;
        }


        if (tox_friend_get_connection_status(tox2, 0, 0) == TOX_CONNECTION_UDP
                && tox_friend_get_connection_status(tox3, 0, 0) == TOX_CONNECTION_UDP)
            break;

        c_sleep(50);
    }

    printf("tox clients connected took %llu seconds\n", time(NULL) - con_time);
    to_compare = 974536;
    tox_callback_friend_message(tox3, print_message, &to_compare);
    uint8_t msgs[TOX_MAX_MESSAGE_LENGTH + 1];
    memset(msgs, 'G', sizeof(msgs));
    TOX_ERR_SEND_MESSAGE errm;
    tox_send_message(tox2, 0, msgs, TOX_MAX_MESSAGE_LENGTH + 1, &errm);
    ck_assert_msg(errm == TOX_ERR_SEND_MESSAGE_TOO_LONG, "TOX_MAX_MESSAGE_LENGTH is too small\n");
    tox_send_message(tox2, 0, msgs, TOX_MAX_MESSAGE_LENGTH, &errm);
    ck_assert_msg(errm == TOX_ERR_SEND_MESSAGE_OK, "TOX_MAX_MESSAGE_LENGTH is too big\n");

    while (1) {
        messages_received = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (messages_received)
            break;

        c_sleep(50);
    }

    printf("tox clients messaging succeeded\n");

    tox_callback_friend_name(tox3, print_nickchange, &to_compare);
    TOX_ERR_SET_INFO err_n;
    bool succ = tox_self_set_name(tox2, (uint8_t *)"Gentoo", sizeof("Gentoo"), &err_n);
    ck_assert_msg(succ && err_n == TOX_ERR_SET_INFO_OK, "tox_self_set_name failed because %u\n", err_n);

    while (1) {
        name_changes = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (name_changes)
            break;

        c_sleep(50);
    }

    ck_assert_msg(tox_friend_get_name_size(tox3, 0, 0) == sizeof("Gentoo"), "Name length not correct");
    uint8_t temp_name[sizeof("Gentoo")];
    tox_friend_get_name(tox3, 0, temp_name, 0);
    ck_assert_msg(memcmp(temp_name, "Gentoo", sizeof("Gentoo")) == 0, "Name not correct");

    tox_callback_friend_typing(tox2, &print_typingchange, &to_compare);
    tox_self_set_typing(tox3, 0, 1, 0);

    while (1) {
        typing_changes = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);


        if (typing_changes == 2)
            break;
        else
            ck_assert_msg(typing_changes == 0, "Typing fail");

        c_sleep(50);
    }

    ck_assert_msg(tox_friend_get_typing(tox2, 0, 0) == 1, "Typing fail");
    tox_self_set_typing(tox3, 0, 0, 0);

    while (1) {
        typing_changes = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (typing_changes == 1)
            break;
        else
            ck_assert_msg(typing_changes == 0, "Typing fail");

        c_sleep(50);
    }

    TOX_ERR_FRIEND_QUERY err_t;
    ck_assert_msg(tox_friend_get_typing(tox2, 0, &err_t) == 0, "Typing fail");
    ck_assert_msg(err_t == TOX_ERR_FRIEND_QUERY_OK, "Typing fail");

    uint32_t packet_number = 160;
    tox_callback_friend_lossless_packet(tox3, &handle_custom_packet, &packet_number);
    uint8_t data_c[TOX_MAX_CUSTOM_PACKET_SIZE + 1];
    memset(data_c, ((uint8_t)packet_number), sizeof(data_c));
    int ret = tox_send_lossless_packet(tox2, 0, data_c, sizeof(data_c), 0);
    ck_assert_msg(ret == 0, "tox_send_lossless_packet bigger fail %i", ret);
    ret = tox_send_lossless_packet(tox2, 0, data_c, TOX_MAX_CUSTOM_PACKET_SIZE, 0);
    ck_assert_msg(ret == 1, "tox_send_lossless_packet fail %i", ret);

    while (1) {
        custom_packet = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (custom_packet == 1)
            break;
        else
            ck_assert_msg(custom_packet == 0, "Lossless packet fail");

        c_sleep(50);
    }

    packet_number = 200;
    tox_callback_friend_lossy_packet(tox3, &handle_custom_packet, &packet_number);
    memset(data_c, ((uint8_t)packet_number), sizeof(data_c));
    ret = tox_send_lossy_packet(tox2, 0, data_c, sizeof(data_c), 0);
    ck_assert_msg(ret == 0, "tox_send_lossy_packet bigger fail %i", ret);
    ret = tox_send_lossy_packet(tox2, 0, data_c, TOX_MAX_CUSTOM_PACKET_SIZE, 0);
    ck_assert_msg(ret == 1, "tox_send_lossy_packet fail %i", ret);

    while (1) {
        custom_packet = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (custom_packet == 1)
            break;
        else
            ck_assert_msg(custom_packet == 0, "lossy packet fail");

        c_sleep(50);
    }

    filenum = file_accepted = file_size = file_sent = sendf_ok = size_recv = 0;
    long long unsigned int f_time = time(NULL);
    tox_callback_file_data(tox3, write_file, &to_compare);
    tox_callback_file_control(tox2, file_print_control, &to_compare);
    tox_callback_file_control(tox3, file_print_control, &to_compare);
    tox_callback_file_send_request(tox3, file_request_accept, &to_compare);
    uint64_t totalf_size = 100 * 1024 * 1024;
    int fnum = tox_new_file_sender(tox2, 0, totalf_size, (uint8_t *)"Gentoo.exe", sizeof("Gentoo.exe"));
    ck_assert_msg(fnum != -1, "tox_new_file_sender fail");
    int fpiece_size = tox_file_data_size(tox2, 0);
    uint8_t f_data[fpiece_size];
    uint8_t num = 0;
    memset(f_data, num, fpiece_size);

    while (1) {
        file_sent = 0;
        tox_iteration(tox1);
        tox_iteration(tox2);
        tox_iteration(tox3);

        if (sendf_ok)
            while (tox_file_send_data(tox2, 0, fnum, f_data, fpiece_size < totalf_size ? fpiece_size : totalf_size) == 0) {
                if (totalf_size <= fpiece_size) {
                    sendf_ok = 0;
                    tox_file_send_control(tox2, 0, 0, fnum, TOX_FILECONTROL_FINISHED, NULL, 0);
                }

                ++num;
                memset(f_data, num, fpiece_size);

                totalf_size -= fpiece_size;
            }

        if (file_sent && size_recv == file_size)
            break;

        uint32_t tox1_interval = tox_iteration_interval(tox1);
        uint32_t tox2_interval = tox_iteration_interval(tox2);
        uint32_t tox3_interval = tox_iteration_interval(tox3);

        if (tox2_interval > tox3_interval) {
            c_sleep(tox3_interval);
        } else {
            c_sleep(tox2_interval);
        }
    }

    printf("100MB file sent in %llu seconds\n", time(NULL) - f_time);

    printf("test_few_clients succeeded, took %llu seconds\n", time(NULL) - cur_time);

    tox_kill(tox1);
    tox_kill(tox2);
    tox_kill(tox3);
}
END_TEST

#define NUM_TOXES 66
#define NUM_FRIENDS 50

START_TEST(test_many_clients)
{
    long long unsigned int cur_time = time(NULL);
    Tox *toxes[NUM_TOXES];
    uint32_t i, j;
    uint32_t to_comp = 974536;

    for (i = 0; i < NUM_TOXES; ++i) {
        toxes[i] = tox_new(0, 0, 0, 0);
        ck_assert_msg(toxes[i] != 0, "Failed to create tox instances %u", i);
        tox_callback_friend_request(toxes[i], accept_friend_request, &to_comp);
    }

    struct {
        uint16_t tox1;
        uint16_t tox2;
    } pairs[NUM_FRIENDS];

    uint8_t address[TOX_ADDRESS_SIZE];

    for (i = 0; i < NUM_FRIENDS; ++i) {
loop_top:
        pairs[i].tox1 = rand() % NUM_TOXES;
        pairs[i].tox2 = (pairs[i].tox1 + rand() % (NUM_TOXES - 1) + 1) % NUM_TOXES;

        for (j = 0; j < i; ++j) {
            if (pairs[j].tox2 == pairs[i].tox1 && pairs[j].tox1 == pairs[i].tox2)
                goto loop_top;
        }

        tox_self_get_address(toxes[pairs[i].tox1], address);

        TOX_ERR_FRIEND_ADD test;
        uint32_t num = tox_friend_add(toxes[pairs[i].tox2], address, (uint8_t *)"Gentoo", 7, &test);

        if (test == TOX_ERR_FRIEND_ADD_ALREADY_SENT) {
            goto loop_top;
        }

        ck_assert_msg(num != UINT32_MAX && test == TOX_ERR_FRIEND_ADD_OK, "Failed to add friend error code: %i", test);
    }

    while (1) {
        uint16_t counter = 0;

        for (i = 0; i < NUM_TOXES; ++i) {
            for (j = 0; j < tox_friend_list_size(toxes[i]); ++j)
                if (tox_friend_get_connection_status(toxes[i], j, 0) == TOX_CONNECTION_UDP)
                    ++counter;
        }

        if (counter == NUM_FRIENDS * 2) {
            break;
        }

        for (i = 0; i < NUM_TOXES; ++i) {
            tox_iteration(toxes[i]);
        }

        c_sleep(50);
    }

    for (i = 0; i < NUM_TOXES; ++i) {
        tox_kill(toxes[i]);
    }

    printf("test_many_clients succeeded, took %llu seconds\n", time(NULL) - cur_time);
}
END_TEST

#define NUM_GROUP_TOX 32

void g_accept_friend_request(Tox *m, const uint8_t *public_key, const uint8_t *data, size_t length, void *userdata)
{
    if (*((uint32_t *)userdata) != 234212)
        return;

    if (length == 7 && memcmp("Gentoo", data, 7) == 0) {
        tox_friend_add_norequest(m, public_key, 0);
    }
}

static Tox *invite_tox;
static unsigned int invite_counter;

void print_group_invite_callback(Tox *tox, int32_t friendnumber, uint8_t type, const uint8_t *data, uint16_t length,
                                 void *userdata)
{
    if (*((uint32_t *)userdata) != 234212)
        return;

    if (type != TOX_GROUPCHAT_TYPE_TEXT)
        return;

    int g_num;

    if ((g_num = tox_join_groupchat(tox, friendnumber, data, length)) == -1)
        return;

    ck_assert_msg(g_num == 0, "Group number was not 0");
    ck_assert_msg(tox_join_groupchat(tox, friendnumber, data, length) == -1,
                  "Joining groupchat twice should be impossible.");

    invite_tox = tox;
    invite_counter = 4;
}

static unsigned int num_recv;

void print_group_message(Tox *tox, int groupnumber, int peernumber, const uint8_t *message, uint16_t length,
                         void *userdata)
{
    if (*((uint32_t *)userdata) != 234212)
        return;

    if (length == (sizeof("Install Gentoo") - 1) && memcmp(message, "Install Gentoo", sizeof("Install Gentoo") - 1) == 0) {
        ++num_recv;
    }
}

START_TEST(test_many_group)
{
    long long unsigned int cur_time = time(NULL);
    Tox *toxes[NUM_GROUP_TOX];
    unsigned int i, j, k;

    uint32_t to_comp = 234212;

    for (i = 0; i < NUM_GROUP_TOX; ++i) {
        toxes[i] = tox_new(0, 0, 0, 0);
        ck_assert_msg(toxes[i] != 0, "Failed to create tox instances %u", i);
        tox_callback_friend_request(toxes[i], &g_accept_friend_request, &to_comp);
        tox_callback_group_invite(toxes[i], &print_group_invite_callback, &to_comp);
    }

    uint8_t address[TOX_ADDRESS_SIZE];
    tox_self_get_address(toxes[NUM_GROUP_TOX - 1], address);

    for (i = 0; i < NUM_GROUP_TOX; ++i) {
        ck_assert_msg(tox_friend_add(toxes[i], address, (uint8_t *)"Gentoo", 7, 0) == 0, "Failed to add friend");

        tox_self_get_address(toxes[i], address);
    }

    while (1) {
        for (i = 0; i < NUM_GROUP_TOX; ++i) {
            if (tox_friend_get_connection_status(toxes[i], 0, 0) != TOX_CONNECTION_UDP) {
                break;
            }
        }

        if (i == NUM_GROUP_TOX)
            break;

        for (i = 0; i < NUM_GROUP_TOX; ++i) {
            tox_iteration(toxes[i]);
        }

        c_sleep(50);
    }

    printf("friends connected, took %llu seconds\n", time(NULL) - cur_time);

    ck_assert_msg(tox_add_groupchat(toxes[0]) != -1, "Failed to create group");
    ck_assert_msg(tox_invite_friend(toxes[0], 0, 0) == 0, "Failed to invite friend");
    ck_assert_msg(tox_group_set_title(toxes[0], 0, "Gentoo", sizeof("Gentoo") - 1) == 0, "Failed to set group title");
    invite_counter = ~0;

    unsigned int done = ~0;
    done -= 5;

    while (1) {
        for (i = 0; i < NUM_GROUP_TOX; ++i) {
            tox_iteration(toxes[i]);
        }

        if (!invite_counter) {
            ck_assert_msg(tox_invite_friend(invite_tox, 0, 0) == 0, "Failed to invite friend");
        }

        if (done == invite_counter) {
            break;
        }

        --invite_counter;
        c_sleep(50);
    }

    for (i = 0; i < NUM_GROUP_TOX; ++i) {
        int num_peers = tox_group_number_peers(toxes[i], 0);
        ck_assert_msg(num_peers == NUM_GROUP_TOX, "Bad number of group peers. expected: %u got: %i, tox %u", NUM_GROUP_TOX,
                      num_peers, i);

        uint8_t title[2048];
        int ret = tox_group_get_title(toxes[i], 0, title, sizeof(title));
        ck_assert_msg(ret == sizeof("Gentoo") - 1, "Wrong title length");
        ck_assert_msg(memcmp("Gentoo", title, ret) == 0, "Wrong title");
    }

    printf("group connected\n");

    for (i = 0; i < NUM_GROUP_TOX; ++i) {
        tox_callback_group_message(toxes[i], &print_group_message, &to_comp);
    }

    ck_assert_msg(tox_group_message_send(toxes[rand() % NUM_GROUP_TOX], 0, (uint8_t *)"Install Gentoo",
                                         sizeof("Install Gentoo") - 1) == 0, "Failed to send group message.");
    num_recv = 0;

    for (j = 0; j < 20; ++j) {
        for (i = 0; i < NUM_GROUP_TOX; ++i) {
            tox_iteration(toxes[i]);
        }

        c_sleep(50);
    }

    ck_assert_msg(num_recv == NUM_GROUP_TOX, "Failed to recv group messages.");

    for (k = NUM_GROUP_TOX; k != 0 ; --k) {
        tox_del_groupchat(toxes[k - 1], 0);

        for (j = 0; j < 10; ++j) {
            for (i = 0; i < NUM_GROUP_TOX; ++i) {
                tox_iteration(toxes[i]);
            }

            c_sleep(50);
        }

        for (i = 0; i < (k - 1); ++i) {
            int num_peers = tox_group_number_peers(toxes[i], 0);
            ck_assert_msg(num_peers == (k - 1), "Bad number of group peers. expected: %u got: %i, tox %u", (k - 1), num_peers, i);
        }
    }

    for (i = 0; i < NUM_GROUP_TOX; ++i) {
        tox_kill(toxes[i]);
    }

    printf("test_many_group succeeded, took %llu seconds\n", time(NULL) - cur_time);
}
END_TEST

Suite *tox_suite(void)
{
    Suite *s = suite_create("Tox");

    DEFTESTCASE(one);
    DEFTESTCASE_SLOW(few_clients, 50);
    DEFTESTCASE_SLOW(many_clients, 150);
    DEFTESTCASE_SLOW(many_group, 100);
    return s;
}

int main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));

    Suite *tox = tox_suite();
    SRunner *test_runner = srunner_create(tox);

    int number_failed = 0;
    srunner_run_all(test_runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(test_runner);

    srunner_free(test_runner);

    return number_failed;
}

