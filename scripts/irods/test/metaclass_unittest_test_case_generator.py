class MetaclassUnittestTestCaseGenerator(type):

    def __new__(meta, name, bases, dct):
        test_generators = [v for k, v in dct.items() if k.startswith('generate_tests')]
        for g in test_generators:
            for method_name, method in g():
                method.__name__ = method_name
                dct[method_name] = method
        return super(MetaclassUnittestTestCaseGenerator, meta).__new__(meta, name, bases, dct)
