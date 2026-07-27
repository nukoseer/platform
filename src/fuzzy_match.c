
#define FUZZY_MATCH_INVALID_SCORE (-(1 << 28))

typedef enum fuzzy_match_bonus_t
{
    FUZZY_MATCH_SEQUENTIAL_BONUS    =  15,
    FUZZY_MATCH_FIRST_LETTER_BONUS  =  15,
    FUZZY_MATCH_SEPARATOR_BONUS     =  30,
    FUZZY_MATCH_CAMELCASE_BONUS     =  30,
    FUZZY_MATCH_LEADING_PENALTY     =  -5,
    FUZZY_MATCH_MAX_LEADING_PENALTY = -15,
    FUZZY_MATCH_UNMATCHED_PENALTY   =  -1,
    FUZZY_MATCH_BASE_SCORE          = 100,
} fuzzy_match_bonus_t;

static u8 fuzzy_match_lower(u8 c)
{
    u8 result = c >= 'A' && c <= 'Z' ? (c + 32) : c;

    return result;
}

static bool fuzzy_match_is_separator(u8 c)
{
    bool result = (c == ' ' || c == '-' || c == '_' || c == '.' || c == ',' || c == ':' || c == '\'' || c == '/');

    return result;
}

static i32 fuzzy_match_position_bonus(const char* text, i32 index)
{
    i32 result = 0;

    if (index > 0)
    {
        u8 prev = text[index - 1];
        u8 curr = text[index];

        if (fuzzy_match_is_separator(prev))
        {
            result += FUZZY_MATCH_SEPARATOR_BONUS;
        }

        if (prev >= 'a' && prev <= 'z' && curr >= 'A' && curr <= 'Z')
        {
            result += FUZZY_MATCH_CAMELCASE_BONUS;
        }
    }

    return result;
}

static bool fuzzy_match(memory_arena_t* arena, const char* pattern, i32 pattern_length, const char* text, i32 text_length, i32* out_score)
{
    bool result = false;

    if (pattern || text || pattern_length > 0 || text_length > 0 ||
        out_score || pattern_length <= text_length)
    {
        memory_arena_span_t span = ma_span_begin(arena);

        i32* match_scores = ma_push_size_zero(span.memory_arena, pattern_length * text_length * sizeof(i32));
        i32* pattern_indices = ma_push_size_zero(span.memory_arena, pattern_length * text_length * sizeof(i32));

        for (i32 i = 0; i < text_length; ++i)
        {
            if (fuzzy_match_lower(pattern[0]) == fuzzy_match_lower(text[i]))
            {
                i32 leading = FUZZY_MATCH_LEADING_PENALTY * i;

                if (leading < FUZZY_MATCH_MAX_LEADING_PENALTY)
                {
                    leading = FUZZY_MATCH_MAX_LEADING_PENALTY;
                }

                i32 gap = FUZZY_MATCH_UNMATCHED_PENALTY * i;
                i32 bonus = i == 0 ? FUZZY_MATCH_FIRST_LETTER_BONUS : fuzzy_match_position_bonus(text, i);
                i32 score = bonus + leading + gap;

                match_scores[i] = score;
                pattern_indices[i] = -1;
            }
            else
            {
                match_scores[i] = FUZZY_MATCH_INVALID_SCORE;
                pattern_indices[i] = -1;
            }
        }
    
        for (i32 i = 1; i < pattern_length; ++i)
        {
            i32 prev_row = (i - 1) * text_length;
            i32 curr_row = i * text_length;
        
            for (i32 j = 0; j < text_length; ++j)
            {
                match_scores[curr_row + j] = FUZZY_MATCH_INVALID_SCORE;
                pattern_indices[curr_row + j] = -1;

                if (fuzzy_match_lower(pattern[i]) != fuzzy_match_lower(text[j]))
                {
                    continue;
                }

                i32 bonus = fuzzy_match_position_bonus(text, j);
                i32 best_match_score = FUZZY_MATCH_INVALID_SCORE;
                i32 best_k = -1;

                for (i32 k = 0; k < j; ++k)
                {
                    if (match_scores[prev_row + k] == FUZZY_MATCH_INVALID_SCORE)
                    {
                        continue;
                    }

                    i32 gap = (j - k - 1) * FUZZY_MATCH_UNMATCHED_PENALTY;
                    i32 sequential = (k == j - 1) ? FUZZY_MATCH_SEQUENTIAL_BONUS : 0;
                    i32 candidate_score = match_scores[prev_row + k] + bonus + gap + sequential;

                    if (candidate_score > best_match_score)
                    {
                        best_match_score = candidate_score;
                        best_k = k;
                    }
                }

                if (best_match_score != FUZZY_MATCH_INVALID_SCORE)
                {
                    match_scores[curr_row + j] = best_match_score;
                    pattern_indices[curr_row + j] = best_k;
                }
            }
        }

        i32 last_row = (pattern_length - 1) * text_length;
        i32 best_score = FUZZY_MATCH_INVALID_SCORE;
        i32 best_index = -1;

        for (i32 i = 0; i < text_length; ++i)
        {
            i32 match_score = match_scores[last_row + i];

            if (match_score != FUZZY_MATCH_INVALID_SCORE && match_score > best_score)
            {
                best_score = match_score;
                best_index = i;
            }
        }

        if (best_index >= 0)
        {
            *out_score = FUZZY_MATCH_BASE_SCORE + best_score;
            result = true;
        }

        ma_span_end(span);
    }

    return result;
}
