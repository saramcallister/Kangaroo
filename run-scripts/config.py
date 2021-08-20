import copy, types

# Fuck libconfig and its long long int pickyness...
class Long:
    def __init__(self, n):
        self.n = n
    def __str__(self):
        return str(self.n) + "L"
    def __int__(self):
        return self.n
    def __float__(self):
        return float(self.n)
    def __eq__(self, other):
        try:
            return self.n == other.n
        except Exception:
            return False

class Config:
    def __init__(self, level=0):
        self.m = {}
        self.level = level

    def __getitem__(self, key):
        # Make it possible to index in dot format
        if "." in key:
            keys = key.split(".")
            curKey = keys[0]
            restKeys = ".".join(keys[1:])
            return self[curKey][restKeys]
        else:
            if key not in list(self.m.keys()):
                self.m[key] = Config(level=self.level+1)
            return self.m[key]

    def __setitem__(self, key, val):
        if "." in key:
            keys = key.split(".")
            curKey = keys[0]
            restKeys = ".".join(keys[1:])
            self[curKey][restKeys] = val
        else:
            assert(self.level > 0) # a root config should never be set directly
            self.m[key] = val

    def __delitem__(self, key):
        if "." in key:
            keys = key.split(".")
            curKey = keys[0]
            restKeys = ".".join(keys[1:])
            del (self[curKey])[restKeys]
        else:
            assert(self.level > 0) # a root config should never be del directly
            del self.m[key]
    
    def pop(self, key):
        if "." in key:
            keys = key.split(".")
            curKey = keys[0]
            restKeys = ".".join(keys[1:])
            self[curKey].pop(restKeys)
        else:
            assert(self.level > 0) # a root config should never be deleted
            self.m.pop(key)

    def __contains__(self, key):
        if "." in key:
            keys = key.split(".")
            curKey = keys[0]
            if curKey in self.m:
                restKeys = ".".join(keys[1:])
                return restKeys in self.m[curKey]
            else:
                return False
        else:
            return key in self.m

    def __str__(self):
        res = ""
        indent = self.level*2
        prefix = "".join([" " for i in range(indent)])
        sKeys = list(self.m.keys())
        sKeys.sort()
        for key in sKeys:
            val = self.m[key]
            if val.__class__.__name__ == "Config": #type(val)==type(Config()):
                res += prefix + key  + " = {\n" + str(val)
                res += prefix + "};\n" 
            else:
                if type(val)==type(""):
                    val = '"' + val + '"'
                else:
                    val = str(val)
                res += prefix + key + " = " + val + ";\n"
        return res

    def __eq__(self, other):
        if not set(self.m.keys()) == set(other.m.keys()):
            return False
        for key in list(self.m.keys()):
            if not self.m[key] == other.m[key]:
                return False
        return True

    def __ne__(self, other):
        return not self == other

    def diff(self, other):
        selfSet = set(self.m.keys())
        otherSet = set(other.m.keys())
        diffKeys = selfSet.symmetric_difference(otherSet)
        diffVals = set()
        for key in sorted(list(selfSet.intersection(otherSet))):
            val = self.m[key]
            if type(val) is types.InstanceType and val.__class__.__name__ == "Config":
                (ck, cv) = val.diff(other.m[key])
                diffKeys |= set([key + "." + v for v in ck])
                diffVals |= set([key + "." + v for v in cv])
            elif not val == other.m[key]:
                diffVals.add(key)
        return (diffKeys, diffVals)

    # NOTE: This is NOT a full parser for the libconfig format, it just re-parses what it produces correctly
    def parse(self, str):
        canonicalStr = str.replace(": \n", "= ") # out.cfg aggregates have the : syntax
        canonicalStr = canonicalStr.replace("false;", "False;")
        canonicalStr = canonicalStr.replace("true;", "True;")
        #print canonicalStr
        level = 0
        stack = []

        for line in canonicalStr.split("\n"):
            # Strip all comments & whitespace
            l = line.split("#")[0]
            #l = l.split("//")[0] # heh, guess what happens when a path has two slashes? Yeah...
            l = l.lstrip().rstrip()
            assert level >= 0

            if l.startswith("#") or len(l) == 0:
                continue
            elif l.endswith("{") and l.find(" = ")  > 0:
                ls = l.split("=")
                name = ls[0].rstrip()
                stack.append(name)
            elif l.endswith("};"):
                stack.pop()
            else:
                if not l.endswith(";"): print("ERROR on parsing", l, " in:\n", canonicalStr, "\nPARSE ERROR (see above)!")
                #assert l.endswith(";")
                ls = l.rstrip(";").split("=")
                name = ls[0].rstrip()
                long = False
                if ls[1].endswith("L"): 
                    long = True
                    ls[1] = ls[1][:-1]
                val = eval("=".join(ls[1:]))
                if long: val = Long(val)
                fn = ".".join(stack) + "." + name
                self[fn] = val

    def clone(self):
        return copy.deepcopy(self)

    # I/O
    def parseFromFile(self, filename):
        with open(filename, 'r') as file:
            self.parse(file.read())
        return self

    def writeToFile(self, filename):
        with open(filename, 'w') as file:
            file.write(str(self))
