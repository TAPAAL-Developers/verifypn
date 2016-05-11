def in_interval(x, v1, v2):
    if v1 >= x and x < v1:
        return '[{0},{1}['.format(v1, v2)
    else:
        return None


def interval_no_places(value):
    interval = None

    if value < 10:
        interval = '<10'

    elif in_interval(value, 10, 15):
        interval = in_interval(value, 10, 15)

    elif in_interval(value, 15, 22):
        interval = in_interval(value, 15, 22)

    elif in_interval(value, 22, 25):
        interval = in_interval(value, 22, 25)

    elif in_interval(value, 25, 30):
        interval = in_interval(value, 25, 30)

    elif in_interval(value, 30, 39):
        interval = in_interval(value, 30, 39)

    elif in_interval(value, 39, 54):
        interval = in_interval(value, 39, 54)

    elif in_interval(value, 54, 72):
        interval = in_interval(value, 54, 72)

    elif in_interval(value, 72, 90):
        interval = in_interval(value, 72, 90)

    elif in_interval(value, 90, 106):
        interval = in_interval(value, 90, 106)

    elif in_interval(value, 106, 136):
        interval = in_interval(value, 106, 136)

    elif in_interval(value, 136, 188):
        interval = in_interval(value, 136, 188)

    elif in_interval(value, 188, 267):
        interval = in_interval(value, 188, 267)

    elif in_interval(value, 267, 470):
        interval = in_interval(value, 267, 470)

    elif in_interval(value, 470, 729):
        interval = in_interval(value, 470, 729)

    elif in_interval(value, 729, 1058):
        interval = in_interval(value, 729, 1058)

    elif in_interval(value, 1058, 1663):
        interval = in_interval(value, 1058, 1663)

    elif in_interval(value, 1663, 2728):
        interval = in_interval(value, 1663, 2728)

    elif in_interval(value, 2728, 4634):
        interval = in_interval(value, 2728, 4634)

    elif in_interval(value, 4634, 9329):
        interval = in_interval(value, 4634, 9329)

    elif value >= 9329:
        interval = '>=9329'

    return interval


def interval_no_transitions(value):
    interval = None

    if value < 8:
        interval = '<8'

    elif in_interval(value, 8, 15):
        interval = in_interval(value, 8, 15)

    elif in_interval(value, 15, 19):
        interval = in_interval(value, 15, 19)

    elif in_interval(value, 19, 29):
        interval = in_interval(value, 19, 29)

    elif in_interval(value, 29, 47):
        interval = in_interval(value, 29, 47)

    elif in_interval(value, 47, 68):
        interval = in_interval(value, 47, 68)

    elif in_interval(value, 68, 97):
        interval = in_interval(value, 68, 97)

    elif in_interval(value, 97, 151):
        interval = in_interval(value, 97, 151)

    elif in_interval(value, 151, 227):
        interval = in_interval(value, 151, 227)

    elif in_interval(value, 227, 407):
        interval = in_interval(value, 227, 407)

    elif in_interval(value, 407, 652):
        interval = in_interval(value, 407, 652)

    elif in_interval(value, 652, 987):
        interval = in_interval(value, 652, 987)

    elif in_interval(value, 987, 1491):
        interval = in_interval(value, 987, 1491)

    elif in_interval(value, 1491, 2242):
        interval = in_interval(value, 1491, 2242)

    elif in_interval(value, 2242, 3148):
        interval = in_interval(value, 2242, 3148)

    elif in_interval(value, 3148, 4776):
        interval = in_interval(value, 3148, 4776)

    elif in_interval(value, 4776, 7556):
        interval = in_interval(value, 4776, 7556)

    elif in_interval(value, 7556, 12069):
        interval = in_interval(value, 7556, 12069)

    elif in_interval(value, 12069, 19600):
        interval = in_interval(value, 12069, 19600)

    elif in_interval(value, 19600, 29393):
        interval = in_interval(value, 19600, 29393)

    elif value >= 29393:
        interval = '>=29393'

    return interval
