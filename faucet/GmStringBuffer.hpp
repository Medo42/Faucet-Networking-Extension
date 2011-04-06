#pragma once
#include <string>

/*
 * This header contain functions to help returning a string to Game Maker.
 * Since strings are returned as char* and not deleted by GM, you have to
 * take care of that eventually, unless you are returning the contents of
 * an internal buffer of a long-lived object.
 *
 * These functions maintain a single char* buffer for returning strings.
 * The old buffer contents are automatically deleted (not "free"d!) when
 * the new ones are set.
 */

/**
 * Delete the old buffer, set the new one.
 */
const char *replaceStringReturnBuffer(const std::string &str);
