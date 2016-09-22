//=============================================
// This requires npm module jayschema:
//   $ npm install jayschema
//
// This is expected to be run via node.js
//
// From the command line:
//   $ node validator.js <schemafile> <datafile>
//
//=============================================

var fs  = require("fs");
var JaySchema = require("jayschema");

if (process.argv.length != 4){
  console.log("Usage: node validator.js <schemafile> <datafile>");
  process.exit(1);
}
// prepare environment
var topLevelSchema = JSON.parse(fs.readFileSync(process.argv[2]));
var data = JSON.parse(fs.readFileSync(process.argv[3]));
var env = new JaySchema();
// register all referenced schemas
env.register(topLevelSchema);
var missingSchemaNames = env.getMissingSchemas();
while( missingSchemaNames.length > 0 ) {
    missingSchemaNames.forEach(function(schemaName) {
        env.register(JSON.parse(fs.readFileSync(schemaName+".json")));
    });
    missingSchemaNames = env.getMissingSchemas();
}
// validate
env.validate(data, topLevelSchema, function(errors) {
    if (errors) {
        console.error(errors);
        process.exit(1);
    }
    else {
        // console.log('VALIDATION: PASSED');
    }
});
