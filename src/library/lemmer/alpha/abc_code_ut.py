# -*- coding: utf-8 -*-


import unittest

from abc_code import *
from abc_code import _remove_nonchanging, _remove_duplicates


class TestMappingFunctions(unittest.TestCase):
    mapping1 = {'a': 'a'}
    mapping2 = {'A': 'a', 'B': 'b'}
    mapping12 = {'a': 'a', 'A': 'a', 'B': 'b'}
    mapping3 = {'a': 'b', 'b': 'a'}
    mapping23 = {'A': 'b', 'B': 'a', 'a': 'b', 'b': 'a'}
    mapping4 = {'a': 'b', 'b': 'c', 'c': 'd', 'd': 'a'}

    def test_compose(self):
        self.assertEqual(compose(self.mapping1), self.mapping1)
        self.assertEqual(compose(self.mapping1, self.mapping2), self.mapping12)
        self.assertEqual(compose(self.mapping2, self.mapping1), self.mapping12)
        self.assertEqual(compose(self.mapping2, self.mapping3), self.mapping23)
        self.assertEqual(compose(self.mapping3, self.mapping3),
                         {'a': 'a', 'b': 'b'})
        self.assertEqual(compose(self.mapping3, self.mapping4),
                         {'a': 'c', 'b': 'b', 'c': 'd', 'd': 'a'})
        self.assertEqual(compose(self.mapping4, self.mapping4,
                                 self.mapping4, self.mapping4),
                         {'a': 'a', 'b': 'b', 'c': 'c', 'd': 'd'})

    def test_reverse_mapping(self):
        self.assertEqual(reverse_mapping(self.mapping1), self.mapping1)
        self.assertEqual(reverse_mapping(self.mapping2),
                         {'a': 'A', 'b': 'B'})
        self.assertEqual(reverse_mapping(self.mapping3), self.mapping3)

        # double apply is identical
        self.assertEqual(reverse_mapping(reverse_mapping(self.mapping4)),
                         self.mapping4)

    def test_merge(self):
        self.assertEqual(merge(self.mapping1, self.mapping2),
                         {'a': 'a', 'A': 'a', 'B': 'b'})
        self.assertEqual(merge(self.mapping1, {}), self.mapping1)

    def test_remove_nonchanging(self):
        self.assertEqual(_remove_nonchanging(self.mapping1), {})
        self.assertEqual(_remove_nonchanging(self.mapping12), self.mapping2)
        self.assertEqual(_remove_nonchanging(self.mapping23), self.mapping23)
        self.assertEqual(_remove_nonchanging({'A': 'a', 'B': 'b', 'C': 'C'}),
                         self.mapping2)


class TestUtilityFunctions(unittest.TestCase):
    def test_remove_duplicates(self):
        self.assertEqual(list(_remove_duplicates([1, 2, 1, 3])), [1, 2, 3])
        self.assertEqual(list(_remove_duplicates([])), [])
        self.assertEqual(list(_remove_duplicates([1, 1, 1, 1])), [1])
        self.assertEqual(list(_remove_duplicates([1, 2, 3, 4])), [1, 2, 3, 4])
        self.assertEqual(list(_remove_duplicates([1, 1, 2, 2])), [1, 2])


class TestCodeBuilders(unittest.TestCase):
    @staticmethod
    def create_and_call_functions(builder_type, *args, **kwargs):
        """Create builder and call all its functions."""
        builder = builder_type(*args, **kwargs)
        builder.generate_code()
        str(builder)

    def test_no_errors(self):

        """Check all builders for running without crashing."""

        self.create_and_call_functions(ConstantCode, 'int', 'test', 0)
        self.create_and_call_functions(StaticArrayCode,
                                       'const int test', [1, 2], str)
        self.create_and_call_functions(TCharStaticArrayCode, 'test', u'abc')
        self.create_and_call_functions(TCharStaticArrayPackCode,
                                       'test', [u'abc', u'qwerty'])
        self.create_and_call_functions(TTrClassCode,
                                       'test', {u'a': u'b', u'b': u'qwerty'})
        self.create_and_call_functions(TDiacriticsMapClassCode,
                                       'test', [u'abc', u'qwerty'])


if __name__ == '__main__':
    unittest.main()
