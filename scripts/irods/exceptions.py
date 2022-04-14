class IrodsError(Exception):
    pass

class IrodsWarning(IrodsError):
    pass

class IrodsSchemaError(Exception):
    pass
