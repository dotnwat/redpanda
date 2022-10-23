test tcSingleClient [main=client]:
  assert GuaranteedWithDrawProgress in
  (union coordinator, { client });
