#pragma once
typedef enum {
    SUCCESS = 0,

    //����� ������ (1-99)
    ERROR_NULL_POINTER = 1,
    ERROR_INVALID_ARGUMENT = 2,
    ERROR_OUT_OF_MEMORY = 3,
    ERROR_FILE_NO_FOUND = 4,

    //������ ������� (100-199)
    DICT_ERROR_INVALID_WORD = 100,
    DICT_ERROR_DUPLICATE_WORD = 101,
    DICT_ERROR_WORD_TOO_SHORT = 102,
    DICT_ERROR_WORD_TOO_LONG = 103,

    //������ �������� ���� (200-299)
    FIELD_INVALID_COORDINATES = 200,
    FIELD_INVALID_LETTER = 201,
    FIELD_INVALID_CELL = 202,//������ �������� ������ � �����
    FIELD_CELL_OCCUPIED = 203,
    FIELD_CELL_EMPTY = 204,
    FIELD_CELL_NOT_CONNECTED = 205,
    FIELD_WORD_ALREADY_EMPTY = 206,

    //������ ������� ������ (300-399)
    GAME_INVALID_WORD = 300,

    //������ �� (400-499)
    AI_ERROR_NO_MOVES_FOUND = 400,
    AI_ERROR_TIMEOUT = 401

} StatusCode;